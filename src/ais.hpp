#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

// 1/2/3类动态报告（已做单位换算与不可用值标记）
struct DynamicReport {
    uint32_t mmsi = 0;
    bool has_pos = false;
    double lat = 0, lon = 0;      // 度
    bool has_sog = false;
    double sog = 0;               // 节
    bool has_cog = false;
    double cog = 0;               // 度，自北顺时针
};

// 5类静态报告中的船名
struct StaticName {
    uint32_t mmsi = 0;
    std::string name;
};

using AisMessage = std::variant<DynamicReport, StaticName>;

// 解码完整AIS载荷；不支持的类型返回 nullopt
std::optional<AisMessage> decode_ais(const std::string& payload, int fill);
