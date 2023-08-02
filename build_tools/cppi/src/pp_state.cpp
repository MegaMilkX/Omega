#include "pp_state.hpp"


std::unordered_map<std::string, std::shared_ptr<PP_FILE>>& pp_get_file_cache() {
    static std::unordered_map<std::string, std::shared_ptr<PP_FILE>>
        file_map;
    return file_map;
}
PP_FILE* pp_find_cached_file(const std::string& canonical_path) {
    auto& map = pp_get_file_cache();
    auto it = map.find(canonical_path);
    if (it == map.end()) {
        return 0;
    }
    return it->second.get();
}
void pp_add_cached_file(PP_FILE* file) {
    auto& map = pp_get_file_cache();
    map.insert(std::make_pair(file->filename_canonical, std::shared_ptr<PP_FILE>(file)));
}