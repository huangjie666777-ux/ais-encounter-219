#include "sixbit.hpp"

bool sixbit_decode(const std::string& payload, int fill, std::vector<uint8_t>& bits) {
    bits.clear();
    bits.reserve(payload.size() * 6);
    for (char c : payload) {
        int v = c - 48;
        if (v > 40) v -= 8;
        if (v < 0 || v > 63) return false;
        for (int b = 5; b >= 0; --b) bits.push_back(static_cast<uint8_t>((v >> b) & 1));
    }
    if (fill < 0 || fill > 5 || static_cast<size_t>(fill) >= bits.size()) return false;
    bits.resize(bits.size() - static_cast<size_t>(fill));
    return true;
}

uint64_t BitReader::get(size_t pos, size_t len) const {
    if (pos + len > bits_.size() || len > 64) return 0;
    uint64_t v = 0;
    for (size_t i = 0; i < len; ++i) v = (v << 1) | bits_[pos + i];
    return v;
}

int64_t BitReader::gets(size_t pos, size_t len) const {
    uint64_t v = get(pos, len);
    if (len > 0 && len < 64 && ((v >> (len - 1)) & 1ULL))
        v |= (~0ULL) << len;
    return static_cast<int64_t>(v);
}
