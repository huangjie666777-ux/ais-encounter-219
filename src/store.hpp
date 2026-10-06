#pragma once
#include "ais.hpp"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct Dynamic {
    bool has_pos = false;
    double lat = 0, lon = 0;
    bool has_sog = false, has_cog = false;
    double sog = 0, cog = 0;
    std::chrono::steady_clock::time_point rx; // 单调收包时刻
};

struct Vessel {
    std::optional<std::string> name;
    std::optional<Dynamic> dyn;
};

// 按MMSI合并船名与动态状态；新动态报告整体替换旧的（不沿用旧位置）
class Store {
public:
    explicit Store(size_t max_vessels) : max_vessels_(max_vessels) {}

    void apply_dynamic(const DynamicReport& r, std::chrono::steady_clock::time_point now);
    void apply_name(const StaticName& s);
    std::unordered_map<uint32_t, Vessel> snapshot() const;

private:
    size_t max_vessels_;
    mutable std::mutex mu_;
    std::unordered_map<uint32_t, Vessel> vessels_;
};
