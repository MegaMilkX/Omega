#include "strid.hpp"

#include <assert.h>
#include <stdint.h>
#include <map>
#include <unordered_map>

static uint32_t next_strid = 1;
static std::map<std::string, uint32_t> strid_table;
static std::unordered_map<uint32_t, std::string> string_table;

uint32_t getStringId(const char* str) {
    auto it = strid_table.find(str);
    if (it == strid_table.end()) {
        uint32_t id = next_strid++;
        strid_table.insert(std::make_pair(std::string(str), id));
        string_table.insert(std::make_pair(id, std::string(str)));
        return id;
    }
    return it->second;
}
std::string getStringIdString(uint32_t id) {
    auto it = string_table.find(id);
    if (it == string_table.end()) {
        assert(false);
        return "";
    }
    return it->second;
}