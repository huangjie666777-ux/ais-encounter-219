#pragma once
#include <cstdint>
#include <string>

struct Config {
    uint16_t udp_port = 5010;
    uint16_t http_port = 8080;
    uint32_t own_mmsi = 0;
    double origin_lat = 0.0;
    double origin_lon = 0.0;
    double max_age_s = 30.0;     // 有效秒数
    double predict_s = 180.0;    // 预测秒数
    double warn_m = 500.0;       // 警戒米数
    size_t max_groups = 64;          // 未完成分片组上限
    size_t max_group_messages = 9;   // 单组分片数上限
    size_t max_vessels = 256;        // 跟踪船数上限
    size_t max_datagram = 1024;      // 单条UDP报文字节上限
};

Config load_config(const std::string& path);
