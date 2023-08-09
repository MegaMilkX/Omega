#include "attribute_interp.hpp"

#include <unordered_map>
#include <string>


static std::unordered_map<std::string, ATTRIB_TYPE> attrib_types;

void set_attribute_type(const char* name, ATTRIB_TYPE type) {
    attrib_types[name] = type;
}
ATTRIB_TYPE get_attribute_type(const char* name) {
    auto it = attrib_types.find(name);
    if (it == attrib_types.end()) {
        return ATTRIB_TOKENS;
    }
    return it->second;
}