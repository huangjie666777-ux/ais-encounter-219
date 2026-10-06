#include "store.hpp"

void Store::apply_dynamic(const DynamicReport& r, std::chrono::steady_clock::time_point now) {
    std::lock_guard lk(mu_);
    auto it = vessels_.find(r.mmsi);
    if (it == vessels_.end()) {
        if (vessels_.size() >= max_vessels_) return; // 船数上限
        it = vessels_.emplace(r.mmsi, Vessel{}).first;
    }
    Dynamic d;
    d.has_pos = r.has_pos; d.lat = r.lat; d.lon = r.lon;
    d.has_sog = r.has_sog; d.sog = r.sog;
    d.has_cog = r.has_cog; d.cog = r.cog;
    d.rx = now;
    it->second.dyn = d; // 整体替换，缺测不沿用旧值
}

void Store::apply_name(const StaticName& s) {
    std::lock_guard lk(mu_);
    auto it = vessels_.find(s.mmsi);
    if (it == vessels_.end()) {
        if (vessels_.size() >= max_vessels_) return;
        it = vessels_.emplace(s.mmsi, Vessel{}).first;
    }
    it->second.name = s.name;
}

std::unordered_map<uint32_t, Vessel> Store::snapshot() const {
    std::lock_guard lk(mu_);
    return vessels_;
}
