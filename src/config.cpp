#include "config.hpp"
#include <fstream>
#include <stdexcept>
#include <cmath>

Config load_config(const std::string& path) {
    Config cfg;
    if (path.empty()) return cfg;
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open config: " + path);
    std::string line;
    while (std::getline(in, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            auto b = s.find_first_not_of(" \t\r\n");
            auto e = s.find_last_not_of(" \t\r\n");
            s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
        };
        trim(k); trim(v);
        if (k.empty() || v.empty()) continue;
        try {
            if (k == "udp_port") cfg.udp_port = static_cast<uint16_t>(std::stoul(v));
            else if (k == "http_port") cfg.http_port = static_cast<uint16_t>(std::stoul(v));
            else if (k == "own_mmsi") cfg.own_mmsi = static_cast<uint32_t>(std::stoul(v));
            else if (k == "origin_lat") cfg.origin_lat = std::stod(v);
            else if (k == "origin_lon") cfg.origin_lon = std::stod(v);
            else if (k == "max_age_s") cfg.max_age_s = std::stod(v);
            else if (k == "predict_s") cfg.predict_s = std::stod(v);
            else if (k == "warn_m") cfg.warn_m = std::stod(v);
            else if (k == "max_groups") cfg.max_groups = std::stoul(v);
            else if (k == "max_group_messages") cfg.max_group_messages = std::stoul(v);
            else if (k == "max_vessels") cfg.max_vessels = std::stoul(v);
            else if (k == "max_datagram") cfg.max_datagram = std::stoul(v);
        } catch (const std::exception&) {
            throw std::runtime_error("bad config value for " + k + ": " + v);
        }
    }
    if (std::abs(cfg.origin_lat) >= 70.0)
        throw std::runtime_error("origin_lat abs must be < 70 degrees");
    if (cfg.max_age_s <= 0 || cfg.predict_s <= 0 || cfg.warn_m <= 0)
        throw std::runtime_error("max_age_s/predict_s/warn_m must be positive");
    return cfg;
}
