#include "query.hpp"
#include "geo.hpp"
#include <nlohmann/json.hpp>

using nlohmann::json;
using Clock = std::chrono::steady_clock;

static constexpr double kAreaRadiusM = 50000.0; // 船位距原点 50km 内

static double age_s(const Dynamic& d, Clock::time_point now) {
    return std::chrono::duration<double>(now - d.rx).count();
}

static json vessel_json(uint32_t mmsi, const Vessel& v, const Config& cfg, Clock::time_point now) {
    json j;
    j["mmsi"] = mmsi;
    if (v.name) j["name"] = *v.name;
    else j["name"] = nullptr;
    if (!v.dyn) {
        j["status"] = "no_dynamic";
        return j;
    }
    double age = age_s(*v.dyn, now);
    j["age_s"] = std::round(age * 10) / 10;
    if (age > cfg.max_age_s) { j["status"] = "expired"; return j; }
    j["status"] = "ok";
    if (v.dyn->has_pos) {
        j["lat"] = v.dyn->lat;
        j["lon"] = v.dyn->lon;
    } else {
        j["lat"] = nullptr; j["lon"] = nullptr;
        j["pos"] = "missing";
    }
    j["sog_kn"] = v.dyn->has_sog ? json(v.dyn->sog) : json(nullptr);
    j["cog_deg"] = v.dyn->has_cog ? json(v.dyn->cog) : json(nullptr);
    return j;
}

std::string ships_json(const Store& store, const Config& cfg) {
    auto now = Clock::now();
    auto snap = store.snapshot();
    json arr = json::array();
    for (const auto& [mmsi, v] : snap) arr.push_back(vessel_json(mmsi, v, cfg, now));
    json out;
    out["own_mmsi"] = cfg.own_mmsi;
    out["ships"] = std::move(arr);
    return out.dump();
}

std::string encounters_json(const Store& store, const Config& cfg) {
    auto now = Clock::now();
    auto snap = store.snapshot();
    json out;
    out["own_mmsi"] = cfg.own_mmsi;
    out["predict_s"] = cfg.predict_s;
    out["warn_m"] = cfg.warn_m;

    // 本船有效性检查
    const Vessel* own = nullptr;
    auto oit = snap.find(cfg.own_mmsi);
    if (oit != snap.end()) own = &oit->second;
    bool own_ok = own && own->dyn && own->dyn->has_pos &&
                  age_s(*own->dyn, now) <= cfg.max_age_s;
    XY own_p, own_v{0, 0};
    double own_age = 0;
    if (own_ok) {
        own_age = age_s(*own->dyn, now);
        own_p = project(own->dyn->lat, own->dyn->lon, cfg.origin_lat, cfg.origin_lon);
        double d0 = std::hypot(own_p.x, own_p.y);
        if (d0 > kAreaRadiusM) own_ok = false;
        else {
            // 匀速推至查询时刻
            if (own->dyn->has_sog && own->dyn->has_cog)
                own_v = velocity(own->dyn->sog, own->dyn->cog);
            own_p.x += own_v.x * own_age;
            own_p.y += own_v.y * own_age;
        }
    }
    if (!own_ok) {
        out["own_status"] = !own ? "own_missing" :
            (!own->dyn ? "own_no_dynamic" :
             (!own->dyn->has_pos ? "own_pos_missing" :
              (age_s(*own->dyn, now) > cfg.max_age_s ? "own_expired" : "own_out_of_area")));
        out["warning"] = "own ship invalid: encounter assessment unavailable, do not rely for safety";
        out["targets"] = json::array();
        return out.dump();
    }
    out["own_status"] = "ok";

    json targets = json::array();
    for (const auto& [mmsi, v] : snap) {
        if (mmsi == cfg.own_mmsi) continue;
        json t;
        t["mmsi"] = mmsi;
        t["name"] = v.name ? json(*v.name) : json(nullptr);
        if (!v.dyn) { t["status"] = "no_dynamic"; targets.push_back(std::move(t)); continue; }
        double age = age_s(*v.dyn, now);
        t["age_s"] = std::round(age * 10) / 10;
        if (age > cfg.max_age_s) { t["status"] = "expired"; targets.push_back(std::move(t)); continue; }
        if (!v.dyn->has_pos) { t["status"] = "pos_missing"; targets.push_back(std::move(t)); continue; }
        XY p = project(v.dyn->lat, v.dyn->lon, cfg.origin_lat, cfg.origin_lon);
        if (std::hypot(p.x, p.y) > kAreaRadiusM) {
            t["status"] = "out_of_area"; targets.push_back(std::move(t)); continue;
        }
        XY vel{0, 0};
        if (v.dyn->has_sog && v.dyn->has_cog) vel = velocity(v.dyn->sog, v.dyn->cog);
        else t["note"] = "sog/cog missing, assumed stationary";
        // 推至查询同一时刻
        p.x += vel.x * age;
        p.y += vel.y * age;
        auto res = cpa(own_p, own_v, p, vel, cfg.predict_s);
        t["status"] = "ok";
        t["dcpa_m"] = std::round(res.dcpa * 10) / 10;
        t["tcpa_s"] = std::round(res.tcpa * 10) / 10;
        t["alarm"] = res.dcpa <= cfg.warn_m;
        targets.push_back(std::move(t));
    }
    out["targets"] = std::move(targets);
    return out.dump();
}
