#pragma once

#include <stdint.h>
#include <string>


inline uint64_t terrainCellKey(int x, int z) {
    return uint64_t(uint32_t(x)) | (uint64_t(uint32_t(z)) << 32);
}
inline void terrainCellCoordsFromKey(uint64_t key, int& x, int& z) {
    x = int32_t(key);
    z = int32_t(key >> 32);
}

inline char* terrainCellCoordAxisToString(char* out, int32_t v) {
    uint32_t mag = static_cast<uint32_t>(v);
    if (v < 0) {
        *out++ = 'n';
        mag = 0u - mag;
    } else {
        *out++ = 'p';
    }
    return std::to_chars(out, out + 10, mag).ptr;
}
inline bool terrainParseCellCoordAxis(const char*& p, const char* end, int32_t& out) {
    if(p == end) return false;

    bool neg;
    if (*p == 'n') {
        neg = true;
    } else if (*p == 'p') {
        neg = false;
    } else {
        return false;
    }
    ++p;

    uint32_t mag;
    auto [next, ec] = std::from_chars(p, end, mag);
    if (ec != std::errc{}) {
        return false;
    }
    p = next;

    if (neg) {
        if (mag > 2147483648u) {
            return false;
        }
        out = static_cast<int32_t>(0u - mag);
    } else {
        if (mag > 2147483647u) {
            return false;
        }
        out = static_cast<int32_t>(mag);
    }
    return true;
}
inline std::string terrainCellCoordsToString(int x, int z) {
    char buf[24];
    char* p = terrainCellCoordAxisToString(buf, x);
    p = terrainCellCoordAxisToString(p, z);
    return std::string(buf, p);
}
inline std::string terrainCellKeyToString(uint64_t key) {
    int cx = 0, cz = 0;
    terrainCellCoordsFromKey(key, cx, cz);
    return terrainCellCoordsToString(cx, cz);
}
inline bool terrainCellKeyFromString(std::string_view key, int32_t& x, int32_t& z) {
    const char* p = key.data();
    const char* end = p + key.size();
    if (!terrainParseCellCoordAxis(p, end, x)) {
        return false;
    }
    if (!terrainParseCellCoordAxis(p, end, z)) {
        return false;
    }
    if (p != end) {
        return false;
    }
    return true;
}

