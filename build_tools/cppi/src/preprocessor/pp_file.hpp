#pragma once

#include <time.h>
#include <unordered_set>
#include <string>
#include <vector>


struct PP_FILE {
    PP_FILE* parent = 0;
    time_t last_write_time = 0;
    std::unordered_set<PP_FILE*> dependencies;
    std::unordered_set<PP_FILE*> dependants;
    //char_provider char_reader;
    std::string file_name;
    std::string filename_canonical;
    std::string file_dir_abs;
    std::vector<char> buf;
    //std::stack<PP_GROUP> pp_group_stack;
};