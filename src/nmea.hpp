#pragma once
#include <optional>
#include <string>

// 一条解析并校验通过的 !AIVDM 语句
struct NmeaSentence {
    int total = 1;        // 分片总数
    int num = 1;          // 本分片序号(1基)
    std::string seq;      // 分片组序号(多片时非空)
    char channel = 'A';
    std::string payload;  // 六位ASCII载荷
    int fill = 0;         // 填充比特数
};

// 校验XOR校验和、字段、载荷字符与填充位；失败返回 nullopt
std::optional<NmeaSentence> parse_nmea(const std::string& line);
