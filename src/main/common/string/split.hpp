#ifndef SPLIT_H
#define SPLIT_H

#include <string>
#include <vector>

template<typename Out>
inline void strSplit(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

inline std::vector<std::string> strSplit(const std::string &s, char delim) {
    std::vector<std::string> elems;
    strSplit(s, delim, std::back_inserter(elems));
    return elems;
}

#endif
