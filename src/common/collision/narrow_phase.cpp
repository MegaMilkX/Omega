#include "narrow_phase.hpp"


phyManifold* phyNarrowPhase::findCachedManifold(manifold_key_t key) {
    auto it = back->pair_manifold_table.find(key);
    if (it == back->pair_manifold_table.end()) {
        return 0;
    }
    return &back->manifolds[it->second];
}
phyManifold* phyNarrowPhase::createNewManifold(manifold_key_t key) {
    auto it_second =
        front->pair_manifold_table.insert(std::make_pair(key, front->manifolds.count())).first;
    front->manifolds.push_back(phyManifold());
    return &front->manifolds[front->manifolds.count() - 1];
}


void phyNarrowPhase::flipBuffers(int tick_id) {
    this->tick_id = tick_id;

    std::swap(front, back);
    front->pair_manifold_table.clear();
    front->manifolds.clear();
}

phyManifold* phyNarrowPhase::getManifold(phyRigidBody* a, phyRigidBody* b, uint16_t normal_key, const gfxm::vec3& Ninitial, bool use_cached) {
    manifold_key_t key = MAKE_MANIFOLD_KEY(a->id, b->id, normal_key);

    bool is_fresh = false;
    auto it = front->pair_manifold_table.find(key);
    if (it == front->pair_manifold_table.end()) {
        is_fresh = true;
        it = front->pair_manifold_table.insert(std::make_pair(key, front->manifolds.count())).first;
        phyManifold m(key);
        m.initial_normal = Ninitial;
        front->manifolds.push_back(m);
    }

    phyManifold* M = &front->manifolds[it->second];
    M->collider_a = a;
    M->collider_b = b;
    if (use_cached && is_fresh) {
        phyManifold* Mcached = findCachedManifold(key);
        if(Mcached) {
            *M = *Mcached;
        }
    }

    return M;
}

void phyNarrowPhase::addContact(
    phyManifold* manifold,
    const gfxm::vec3& pt_a,
    const gfxm::vec3& pt_b,
    const gfxm::vec3& normal_a,
    const gfxm::vec3& normal_b,
    float distance
) {
    phyContactPoint cp;
    cp.point_a = pt_a;
    cp.point_b = pt_b;
    cp.normal_a = normal_a;
    cp.normal_b = normal_b;
    cp.depth = distance;
    addContact(manifold, cp);
}

void phyNarrowPhase::primeManifold(phyManifold* manifold, phyContactPoint& cp) {
    auto buildTangentBasis = [](const gfxm::vec3& N, gfxm::vec3& t1, gfxm::vec3& t2) {
        gfxm::vec3 ref = (fabsf(N.x) > .9f) ? gfxm::vec3(.0f, 1.f, .0f) : gfxm::vec3(1.f, .0f, .0f);
        t1 = gfxm::normalize(gfxm::cross(N, ref));
        t2 = gfxm::cross(N, t1);
    };
    buildTangentBasis(cp.normal_a, manifold->t1, manifold->t2);
    assert(manifold->t1.length() > .95f);
    assert(manifold->t2.length() > .95f);
    auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
        return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
    };
    gfxm::vec2 p2 = project(manifold->t1, manifold->t2, cp.point_a);
    manifold->extremes.min = p2;
    manifold->extremes.max = p2;
}
void phyNarrowPhase::removeManifold(int i) {
    auto removed_key = front->manifolds[i].key;
    std::swap(front->manifolds[i], front->manifolds[front->manifolds.count() - 1]);
    front->manifolds.resize(front->manifolds.count() - 1);
    front->pair_manifold_table[front->manifolds[i].key] = i;
    front->pair_manifold_table.erase(removed_key);
}
void phyNarrowPhase::rebuildManifold(phyManifold* m) {
    if (m->pointCount() == 0) {
        return;
    }
    primeManifold(m, m->points[0]);
    for (int i = 1; i < m->pointCount(); ++i) {
        auto& cp = m->points[i];
        auto project = [](const gfxm::vec3& t1, const gfxm::vec3& t2, const gfxm::vec3& p)->gfxm::vec2{
                return gfxm::vec2(gfxm::dot(t1, p), gfxm::dot(t2, p));
            };
        gfxm::vec2 p2 = project(m->t1, m->t2, cp.point_a);
        if(p2.x < m->extremes.min.x) { m->iminx = i; }
        if(p2.y < m->extremes.min.y) { m->iminy = i; }
        if(p2.x > m->extremes.max.x) { m->imaxx = i; }
        if(p2.y > m->extremes.max.y) { m->imaxy = i; }
        gfxm::expand(m->extremes, p2);
    }
}

void sortPolygon(phyArray<phyContactPoint, PHY_MAX_MANIFOLD_POINTS>& points) {
    // Find centroid
    gfxm::vec2 C;
    for(int i = 0; i < points.count(); ++i) {
        C += points[i].point2d;
    }
    C /= float(points.count());

    // Sort
    auto fn_angle_less = [C](const gfxm::vec2& p1, const gfxm::vec2& p2)->bool {
        bool ua = p1.y > C.y || p1.y == C.y && p1.x > C.x;
        bool ub = p2.y > C.y || p2.y == C.y && p2.x > C.x;
        if (ua != ub) {
            return ua;
        }
        return (p2.x - p1.x) * (C.y - p1.y) -
            (p2.y - p1.y) * (C.x - p1.x) > .0f;
    };
    
    std::sort(&points[0], &points[0] + points.count(), [C, fn_angle_less](const phyContactPoint& p1, const phyContactPoint& p2)->bool {
        float angA = std::atan2f(p1.point2d.y - C.y, p1.point2d.x - C.x);
        float angB = std::atan2f(p2.point2d.y - C.y, p2.point2d.x - C.x);
        return angA < angB;
        //return fn_angle_less(p1.point2d, p2.point2d);
    });
}

void phyNarrowPhase::addContact3(phyManifold* m, phyContactPoint& cp) {
    /*float EPS_DIST = .02f;
    for (int i = 0; i < m->pointCount(); ++i) {
        phyContactPoint& oldcp = m->points[i];
        if (gfxm::length(oldcp.point_a - cp.point_a) < EPS_DIST
            || gfxm::length(oldcp.point_b - cp.point_b) < EPS_DIST)
        {
            cp.bias = oldcp.bias;
            cp.dbg_vn = oldcp.dbg_vn;
            cp.Jn_acc = oldcp.Jn_acc;
            cp.Jt_acc = oldcp.Jt_acc;
            cp.mass_normal = oldcp.mass_normal;
            cp.mass_tangent1 = oldcp.mass_tangent1;
            cp.mass_tangent2 = oldcp.mass_tangent2;
            cp.tick_id = oldcp.tick_id;
            m->points[i] = cp;
            return;
        }
    }*/
    /*
    gfxm::vec3 centroid(.0f, .0f, .0f);
    for (int i = 0; i < m->points.count(); ++i) {
        centroid += m->points[i].point_a;
    }
    centroid /= float(m->points.count());
    gfxm::vec2 centroid2d = gfxm::vec2(gfxm::dot(m->t1, centroid), gfxm::dot(m->t2, centroid));

    for (int i = 0; i < m->points.count(); ++i) {
        m->points[i].point2d = gfxm::vec2(gfxm::dot(m->t1, m->points[i].point_a), gfxm::dot(m->t2, m->points[i].point_a)) - centroid2d;
    }

    assert(m->t1.length() > .95f);
    assert(m->t2.length() > .95f);
    */
    const gfxm::vec3& p3d = cp.point_a;
    cp.point2d = gfxm::vec2(gfxm::dot(m->t1, p3d), gfxm::dot(m->t2, p3d));// - centroid2d;

    if (m->pointCount() < 2) {
        m->points.push_back(cp);
        phyMakeManifoldHull(m, &m->hull);
        std::sort(&m->points[0], &m->points[0] + m->points.count(), [](const phyContactPoint& a, const phyContactPoint& b) {
            return a.depth > b.depth;
        });
        return;
    }

    if (m->pointCount() < 3) {
        m->points.push_back(cp);
        phyMakeManifoldHull(m, &m->hull);
        std::sort(&m->points[0], &m->points[0] + m->points.count(), [](const phyContactPoint& a, const phyContactPoint& b) {
            return a.depth > b.depth;
        });
        return;
    }

    if (m->hull.isInside(cp.point2d)) {
        return;
    }

    if (m->pointCount() < PHY_MAX_MANIFOLD_POINTS) {
        m->points.push_back(cp);
        phyMakeManifoldHull(m, &m->hull);
        // prune manifold points
        phyArray<phyContactPoint, PHY_MAX_MANIFOLD_POINTS> new_points;
        for (int i = 0; i < m->hull.points.count(); ++i) {
            new_points.push_back(m->points[m->hull.points[i].original_point]);
        }
        m->points = new_points;
        
        std::sort(&m->points[0], &m->points[0] + m->points.count(), [](const phyContactPoint& a, const phyContactPoint& b) {
            return a.depth > b.depth;
        });
        return;
    }

    // Trying to replace an existing point
    int best_idx = -1;
    if(0/*cp.vt.length2() > 1e-6f*/) {
        // Replace by extent along motion
        gfxm::vec3 that3 = gfxm::normalize(cp.vt);
        gfxm::vec2 that(gfxm::dot(m->t1, p3d), gfxm::dot(m->t2, p3d));
        float max_extent = .0f;
        float min_extent = FLT_MAX;
        int min_extent_idx = -1;
        float new_extent = gfxm::dot(that, cp.point2d);
        for (int i = 0; i < PHY_MAX_MANIFOLD_POINTS; ++i) {
            float extent = gfxm::dot(that, m->points[i].point2d);
            if (extent > max_extent) {
                max_extent = extent;
            }
            if (extent < min_extent) {
                min_extent = extent;
                min_extent_idx = i;
            }
        }
        if (new_extent > max_extent) {
            best_idx = min_extent_idx;
        }
    } else {
        // No tangential motion, replace by area
        float best_area = m->hull.area2();
        for (int i = 0; i < m->points.count(); ++i) {
            phyManifoldConvexHull new_hull;
            phyPrimeConvexHull(m, &new_hull);
            new_hull.points[i].at = cp.point2d;

            phyMonotoneChainConvexHull(&new_hull);
            float area = new_hull.area2();
            if (area > best_area) {
                best_idx = i;
                best_area = area;
            }
        }
    }

    if (best_idx >= 0) {
        // NOTE: Experimental
        const auto& oldcp = m->points[best_idx];
        if((oldcp.point2d - cp.point2d).length2() <= .04f) {
            cp.Jn_acc = oldcp.Jn_acc;
            cp.Jt_acc = oldcp.Jt_acc;
        }
        // ==================

        m->points[best_idx] = cp;
        phyMakeManifoldHull(m, &m->hull);

        // prune manifold points
        phyArray<phyContactPoint, PHY_MAX_MANIFOLD_POINTS> new_points;
        for (int i = 0; i < m->hull.points.count(); ++i) {
            new_points.push_back(m->points[m->hull.points[i].original_point]);
        }
        m->points = new_points;

        std::sort(&m->points[0], &m->points[0] + m->points.count(), [](const phyContactPoint& a, const phyContactPoint& b) {
            return a.depth > b.depth;
        });
    }
}

void phyNarrowPhase::addContact(phyManifold* manifold, phyContactPoint& cp) {
    if (!cp.normal_a.is_valid()) {
        //return;
    }

    if (cp.normal_a.length2() < .95f) {
        return;
    }

    if (manifold->pointCount() == 0) {
        primeManifold(manifold, cp);
    }

    cp.lcl_point_a = gfxm::inverse(manifold->collider_a->getTransform()) * gfxm::vec4(cp.point_a, 1.f);
    cp.lcl_point_b = gfxm::inverse(manifold->collider_b->getTransform()) * gfxm::vec4(cp.point_b, 1.f);
    cp.lcl_normal = gfxm::inverse(gfxm::to_mat3(manifold->collider_a->getRotation())) * cp.normal_a;

    cp.tick_id = tick_id;

    addContact3(manifold, cp);
}

