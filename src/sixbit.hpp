#pragma once
#include <cstdint>
#include <string>
#include <vector>

// 六位ASCII载荷解码为比特序列(MSB优先)，去掉末尾 fill 个填充比特
bool sixbit_decode(const std::string& payload, int fill, std::vector<uint8_t>& bits);

class BitReader {
public:
    explicit BitReader(const std::vector<uint8_t>& bits) : bits_(bits) {}
    size_t size() const { return bits_.size(); }
    uint64_t get(size_t pos, size_t len) const;   // 无符号
    int64_t gets(size_t pos, size_t len) const;   // 有符号补码
private:
    const std::vector<uint8_t>& bits_;
};
