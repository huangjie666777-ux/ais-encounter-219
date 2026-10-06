#include "ais.hpp"
#include "sixbit.hpp"

static std::string decode_name(const BitReader& r, size_t pos, size_t chars) {
    std::string out;
    out.reserve(chars);
    for (size_t i = 0; i < chars; ++i) {
        uint64_t v = r.get(pos + i * 6, 6);
        char c = (v < 32) ? static_cast<char>(v + 64) : static_cast<char>(v);
        out.push_back(c);
    }
    // 去掉尾部 '@' 与空格
    while (!out.empty() && (out.back() == '@' || out.back() == ' ')) out.pop_back();
    return out;
}

std::optional<AisMessage> decode_ais(const std::string& payload, int fill) {
    std::vector<uint8_t> bits;
    if (!sixbit_decode(payload, fill, bits)) return std::nullopt;
    BitReader r(bits);
    if (r.size() < 38) return std::nullopt;
    uint64_t type = r.get(0, 6);
    uint32_t mmsi = static_cast<uint32_t>(r.get(8, 30));
    if (mmsi == 0) return std::nullopt;

    if (type >= 1 && type <= 3) {
        if (r.size() < 168) return std::nullopt;
        DynamicReport d;
        d.mmsi = mmsi;
        int64_t lon_raw = r.gets(61, 28);
        int64_t lat_raw = r.gets(89, 27);
        uint64_t sog_raw = r.get(50, 10);
        uint64_t cog_raw = r.get(116, 12);
        // 不可用值: lon=181, lat=91, SOG=1023, COG=3600
        if (lon_raw != 181 * 600000 && lat_raw != 91 * 600000) {
            d.lon = lon_raw / 600000.0;
            d.lat = lat_raw / 600000.0;
            if (d.lat >= -90.0 && d.lat <= 90.0 && d.lon >= -180.0 && d.lon <= 180.0)
                d.has_pos = true;
        }
        if (sog_raw != 1023) { d.sog = sog_raw / 10.0; d.has_sog = true; }
        if (cog_raw != 3600) { d.cog = cog_raw / 10.0; d.has_cog = true; }
        return AisMessage{d};
    }
    if (type == 5) {
        if (r.size() < 232) return std::nullopt;
        StaticName s;
        s.mmsi = mmsi;
        s.name = decode_name(r, 112, 20);
        return AisMessage{s};
    }
    return std::nullopt; // 其他类型忽略
}
