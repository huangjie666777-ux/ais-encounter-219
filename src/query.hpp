#pragma once
#include "config.hpp"
#include "store.hpp"
#include <string>

// 船舶列表 JSON
std::string ships_json(const Store& store, const Config& cfg);
// 本船对各目标会遇 JSON（缺测/过期/本船无效均明确标注）
std::string encounters_json(const Store& store, const Config& cfg);
