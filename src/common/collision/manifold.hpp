#pragma once

#include <assert.h>
#include <vector>
#include "math/gfxm.hpp"
#include "collision/collider.hpp"
#include "collision_contact_point.hpp"


template<typename T, int CAPACITY>
struct phyArray {
    T _data[CAPACITY];
    int _count = 0;

    phyArray(int count = 0) {
        memset(&_data[0], 0, CAPACITY * sizeof(T));
        _count = count;
    }
    phyArray(const std::initializer_list<T>& list) {
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
    const T& back() const {
        return _data[_count - 1];
    }
    T& back() {
        return _data[_count - 1];
    }
    void push_back(const T& item) {
        assert(_count < CAPACITY);
        _data[_count++] = item;
    }
    void pop_back() {
        if (_count > 0) {
            --_count;
        }
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

    phyArray<T, CAPACITY>& operator=(const phyArray<T, CAPACITY>& other) {
        _count = other._count;
        for (int i = 0; i < other._count; ++i) {
            _data[i] = other._data[i];
        }
        return *this;
    }
};

static constexpr int PHY_MAX_MANIFOLD_POINTS = 4;

class phyManifoldConvexHull {
public:
    struct Point {
        gfxm::vec2 at;
        int original_point = -1;
    };
    phyArray<Point, PHY_MAX_MANIFOLD_POINTS> points;

    float area2() const {
        if (points.count() < 3) {
            return .0f;
        }

        gfxm::vec2 centroid(.0f, .0f);
        for (int i = 0; i < points.count(); ++i) {
            centroid += points[i].at;
        }
        centroid /= float(points.count());

        float area = .0f;
        for (int i = 0; i < points.count(); ++i) {
            const auto& a = points[i].at - centroid;
            const auto& b = points[(i + 1) % points.count()].at - centroid;

            // TODO: remove abs, this is done to temporarily eliminate a possible bug that is not there it seems
            area += fabsf(a.x * b.y - b.x * a.y);
        }
        return area;
    }
    bool isInside(const gfxm::vec2& pt) {
        for (int i = 0; i < points.count(); ++i) {
            const auto& a = points[i].at;
            const auto& b = points[(i + 1) % points.count()].at;
            const auto& c = pt;

            float cross = (b.x - a.x) * (c.y - a.y) -
                (b.y - a.y) * (c.x - a.x);
            if (cross < .0f) {
                return false;
            }
        }
        return true;
    }
};

class phyManifold {
public:
    uint64_t key = 0;
    phyArray<phyContactPoint, PHY_MAX_MANIFOLD_POINTS> points;
    phyManifoldConvexHull hull;
    phyRigidBody* collider_a = 0;
    phyRigidBody* collider_b = 0;

    //gfxm::vec3 normal;
    gfxm::vec3 initial_normal;
    float depth = .0f;

    gfxm::vec3 t1;
    gfxm::vec3 t2;
    gfxm::rect extremes;
    int iminx = 0, imaxx = 0;
    int iminy = 0, imaxy = 0;

    phyManifold() {}
    phyManifold(uint64_t key)
        : key(key) {}

    int pointCount() const { return points.count(); }
};


void phyMonotoneChainConvexHull(phyManifoldConvexHull* hull);
void phyPrimeConvexHull(phyManifold* m, phyManifoldConvexHull* hull);
void phyMakeManifoldHull(phyManifold* manifold, phyManifoldConvexHull* hull);




