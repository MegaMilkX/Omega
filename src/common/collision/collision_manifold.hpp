#pragma once

#include <assert.h>
#include <vector>
#include "math/gfxm.hpp"
#include "collision/collider.hpp"
#include "collision_contact_point.hpp"


template<typename T, int CAPACITY>
struct CollisionArray {
    T _data[CAPACITY];
    int _count = 0;

    CollisionArray() {
        memset(&_data[0], 0, CAPACITY * sizeof(T));
    }
    CollisionArray(const std::initializer_list<T>& list) {
        for (auto& e : list) {
            push_back(e);
        }
    }

    constexpr int capacity() const { return CAPACITY; }

    void clear() {
        _count = 0;
    }
    int count() const {
        return _count;
    }
    T* data() {
        return _data;
    }
    void push_back(const T& item) {
        assert(_count < CAPACITY);
        _data[_count++] = item;
    }
    void erase(int at) {
        assert(_count > 0);
        assert(at < _count && at >= 0);
        _data[at] = data[_count - 1];
        --_count;
    }
    void resize(int sz) {
        assert(sz >= 0 && sz <= CAPACITY);
        _count = sz;
    }

    T& operator[](int i) {
        return _data[i];
    }
    const T& operator[](int i) const {
        return _data[i];
    }
};

class CollisionManifold {
public:
    uint64_t key = 0;
    CollisionArray<ContactPoint, 4> points;
    /*
    ContactPoint* points = 0;
    int           point_count = 0;
    ContactPoint* old_points = 0;
    int           old_point_count = 0;
    int           preserved_point_count = 0;*/
    Collider* collider_a = 0;
    Collider* collider_b = 0;

    gfxm::vec3 normal;
    float depth = .0f;

    gfxm::vec3 t1;
    gfxm::vec3 t2;
    gfxm::rect extremes;
    int iminx = 0, imaxx = 0;
    int iminy = 0, imaxy = 0;

    CollisionManifold() {}
    CollisionManifold(uint64_t key)
        : key(key) {}

    int pointCount() const { return points.count(); }
};