#pragma once

#include <stdint.h>
#include "math/gfxm.hpp"
#include "gjk_types.hpp"


constexpr int EPA_MAX_ITERATIONS = 20;
constexpr int EPA_MAX_VERTICES = EPA_MAX_ITERATIONS + 4;
constexpr int EPA_MAX_FACES = 2 * EPA_MAX_VERTICES - 4;
constexpr int EPA_MAX_HORIZON = EPA_MAX_FACES / 2; // EPA_MAX_VERTICES - 2


struct EPA_Face {
    uint16_t a, b, c;
    gfxm::vec3 normal;
    float distance;
};

struct EPA_Edge { 
    uint16_t a;
    uint16_t b;
    bool operator==(const EPA_Edge& other) const {
        return a == other.a && b == other.b;
    }
};

struct EPA_Result {
    bool valid;
    gfxm::vec3 normal;
    float depth;
    gfxm::vec3 contact_a;
    gfxm::vec3 contact_b;
};

template<typename T, int CAPACITY>
struct EPA_Array {
    T data[CAPACITY];
    int count = 0;

    EPA_Array() {}
    EPA_Array(const std::initializer_list<T>& list) {
        for (auto& e : list) {
            push_back(e);
        }
    }

    constexpr int capacity() const { return CAPACITY; }

    void clear() {
        count = 0;
    }
    void push_back(const T& item) {
        assert(count < CAPACITY);
        data[count++] = item;
    }
    void erase(int at) {
        assert(count > 0);
        assert(at < count && at >= 0);
        data[at] = data[count - 1];
        --count;
    }

    T& operator[](int i) {
        return data[i];
    }
    const T& operator[](int i) const {
        return data[i];
    }
};

struct EPA_Context {
    EPA_Array<GJK_SupportPoint, EPA_MAX_VERTICES> points;
    EPA_Array<EPA_Face, EPA_MAX_FACES> faces;
    EPA_Array<EPA_Edge, EPA_MAX_HORIZON> hole_edges;
};

