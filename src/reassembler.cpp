#include "reassembler.hpp"

std::optional<std::pair<std::string, int>>
Reassembler::add(const std::string& endpoint, const NmeaSentence& s, Clock::time_point now) {
    if (s.total == 1) return std::make_pair(s.payload, s.fill); // 单片直接完成
    if (static_cast<size_t>(s.total) > max_group_messages_) return std::nullopt;

    Key key{endpoint, s.seq, s.channel};
    auto it = groups_.find(key);
    if (it == groups_.end()) {
        if (groups_.size() >= max_groups_) return std::nullopt; // 组数上限
        Group g;
        g.total = s.total;
        g.created = now;
        it = groups_.emplace(key, std::move(g)).first;
    }
    Group& g = it->second;
    if (s.total != g.total) { groups_.erase(it); return std::nullopt; } // 总数冲突整组丢弃

    auto [pit, inserted] = g.parts.emplace(s.num, std::make_pair(s.payload, s.fill));
    if (!inserted) {
        if (pit->second.first == s.payload && pit->second.second == s.fill)
            return std::nullopt; // 重复分片：幂等忽略
        groups_.erase(it);       // 冲突：整组丢弃
        return std::nullopt;
    }
    if (static_cast<int>(g.parts.size()) < g.total) return std::nullopt;

    std::string payload;
    int fill = 0;
    for (int i = 1; i <= g.total; ++i) {
        auto p = g.parts.find(i);
        if (p == g.parts.end()) return std::nullopt;
        payload += p->second.first;
        fill = p->second.second; // 填充位取最后一片
    }
    groups_.erase(it);
    return std::make_pair(std::move(payload), fill);
}

void Reassembler::expire(Clock::time_point now, std::chrono::seconds timeout) {
    for (auto it = groups_.begin(); it != groups_.end();) {
        if (now - it->second.created > timeout) it = groups_.erase(it);
        else ++it;
    }
}
