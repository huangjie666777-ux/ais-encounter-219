#include "nmea.hpp"
#include <vector>

static bool sixbit_char_ok(char c) {
    return c >= 0x30 && c <= 0x77 && !(c > 0x57 && c < 0x60);
}

std::optional<NmeaSentence> parse_nmea(const std::string& line) {
    if (line.size() < 10 || line[0] != '!') return std::nullopt;
    auto star = line.rfind('*');
    if (star == std::string::npos || star + 3 > line.size()) return std::nullopt;
    unsigned xor_sum = 0;
    for (size_t i = 1; i < star; ++i) xor_sum ^= static_cast<unsigned char>(line[i]);
    auto hexval = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    int hi = hexval(line[star + 1]), lo = hexval(line[star + 2]);
    if (hi < 0 || lo < 0 || static_cast<unsigned>(hi * 16 + lo) != xor_sum) return std::nullopt;

    std::vector<std::string> f;
    size_t pos = 1;
    while (true) {
        auto comma = line.find(',', pos);
        size_t end = (comma == std::string::npos || comma > star) ? star : comma;
        f.push_back(line.substr(pos, end - pos));
        if (end == star) break;
        pos = comma + 1;
    }
    if (f.size() != 7) return std::nullopt;
    if (f[0] != "AIVDM" && f[0] != "AIVDO") return std::nullopt;

    auto to_int = [](const std::string& s, int& out) {
        if (s.empty()) return false;
        int v = 0;
        for (char c : s) { if (c < '0' || c > '9') return false; v = v * 10 + (c - '0'); }
        out = v; return true;
    };
    NmeaSentence s;
    if (!to_int(f[1], s.total) || !to_int(f[2], s.num)) return std::nullopt;
    if (s.total < 1 || s.total > 9 || s.num < 1 || s.num > s.total) return std::nullopt;
    s.seq = f[3];
    if (s.total > 1 && s.seq.empty()) return std::nullopt; // 多片序号不能为空
    if (s.total == 1 && !s.seq.empty()) return std::nullopt;
    if (f[4].size() != 1) return std::nullopt;
    s.channel = f[4][0];
    if (s.channel != 'A' && s.channel != 'B' && s.channel != '1' && s.channel != '2')
        return std::nullopt;
    s.payload = f[5];
    if (s.payload.empty() || s.payload.size() > 82) return std::nullopt;
    for (char c : s.payload) if (!sixbit_char_ok(c)) return std::nullopt;
    if (!to_int(f[6], s.fill) || s.fill < 0 || s.fill > 5) return std::nullopt;
    return s;
}
