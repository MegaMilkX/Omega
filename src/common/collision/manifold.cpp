#include "manifold.hpp"


static float cross(const gfxm::vec2& o, const gfxm::vec2& a, const gfxm::vec2& b) {
    return (a.x - o.x) * (b.y - o.y) -
        (a.y - o.y) * (b.x - o.x);
}

void phyMonotoneChainConvexHull(phyManifoldConvexHull* chull) {
    constexpr int MAX_POINTS = PHY_MAX_MANIFOLD_POINTS;
    using Point = phyManifoldConvexHull::Point;

    phyArray<Point, MAX_POINTS> sorted = chull->points;
    std::sort(&sorted[0], &sorted[0] + sorted.count(), [](const Point& a, const Point& b) {
        if (a.at.x == b.at.x) {
            return a.at.y < b.at.y;
        }
        return a.at.x < b.at.x;
    });

    phyArray<Point, MAX_POINTS * 2> hull;

    const float EPS = .0f;

    for (int i = 0; i < sorted.count(); ++i) {
        const auto& p = sorted[i];
        while (hull.count() >= 2 && cross(hull[hull.count() - 2].at, hull.back().at, p.at) <= EPS) {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    int lower_count = hull.count();
    for (int i = sorted.count() - 2; i >= 0; --i) {
        const auto& p = sorted[i];
        while (hull.count() > lower_count && cross(hull[hull.count() - 2].at, hull.back().at, p.at) <= EPS) {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    // TODO: Temporary fix to not exceed max points
    while(hull.count() > chull->points.capacity()) {
        hull.pop_back();
    }

    chull->points.resize(hull.count());
    for (int i = 0; i < hull.count(); ++i) {
        chull->points[i] = hull[i];
    }
}
void phyPrimeConvexHull(phyManifold* m, phyManifoldConvexHull* chull) {
    chull->points.resize(m->points.count());
    for (int i = 0; i < m->points.count(); ++i) {
        chull->points[i].at = m->points[i].point2d;
        chull->points[i].original_point = i;
    }
}
void phyMakeManifoldHull(phyManifold* manifold, phyManifoldConvexHull* chull) {
    phyPrimeConvexHull(manifold, chull);

    if (chull->points.count() < 3) {
        return;
    }

    phyMonotoneChainConvexHull(chull);
}

