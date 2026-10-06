#pragma once
#include "nmea.hpp"
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <utility>

// 多片重组：按 (发送端点, 序号, 频道) 隔离；乱序可收，重复幂等，
// 同序号冲突整组丢弃，超时未齐清理。
class Reassembler {
public:
    using Clock = std::chrono::steady_clock;

    Reassembler(size_t max_groups, size_t max_group_messages)
        : max_groups_(max_groups), max_group_messages_(max_group_messages) {}

    // 完整时返回 (拼接载荷, 填充位)
    std::optional<std::pair<std::string, int>> add(const std::string& endpoint,
                                                   const NmeaSentence& s,
                                                   Clock::time_point now);
    void expire(Clock::time_point now, std::chrono::seconds timeout);
    size_t pending_groups() const { return groups_.size(); }

private:
    struct Group {
        int total = 0;
        Clock::time_point created;
        std::map<int, std::pair<std::string, int>> parts; // num -> (payload, fill)
    };
    struct Key {
        std::string endpoint, seq;
        char channel;
        bool operator<(const Key& o) const {
            if (endpoint != o.endpoint) return endpoint < o.endpoint;
            if (seq != o.seq) return seq < o.seq;
            return channel < o.channel;
        }
    };
    size_t max_groups_, max_group_messages_;
    std::map<Key, Group> groups_;
};
