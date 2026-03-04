#pragma once

#include "manifold.hpp"


constexpr int PHY_MAX_MANIFOLDS = 8192;

inline uint32_t phyHash(uint32_t a)
{
    a = (a+0x7ed55d16u) + (a<<12);
    a = (a^0xc761c23cu) ^ (a>>19);
    a = (a+0x165667b1u) + (a<<5);
    a = (a+0xd3a2646cu) ^ (a<<9);
    a = (a+0xfd7046c5u) + (a<<3);
    a = (a^0xb55a4f09u) ^ (a>>16);
    return a;
}

inline uint16_t phyMakeNormalKey(const gfxm::vec3& N) {
    float u = N.x;
    float v = N.y;
    float w = N.z;
    u = (u + 1.f) * .5f;
    v = (v + 1.f) * .5f;
    w = (w + 1.f) * .5f;
    int iu = u * 4.f + .1f;
    int iv = v * 4.f + .1f;
    int iw = w * 4.f + .1f;
    return ((uint16_t(iu) << 10) + (uint16_t(iv) << 5) + uint16_t(w));
}

// 0xBBBBBB AAAAAA NNNN
typedef uint64_t manifold_key_t;
#define MAKE_MANIFOLD_KEY(A, B, NORMAL_KEY) \
    manifold_key_t(uint64_t(NORMAL_KEY & 0x7FFF) | uint64_t(std::min(A, B)) << 16 | (uint64_t(std::max(A, B)) << 40))


struct phyNarrowPhaseData {
    std::unordered_map<manifold_key_t, int> pair_manifold_table;
    phyArray<phyManifold, PHY_MAX_MANIFOLDS> manifolds;
};

class phyNarrowPhase {
    phyNarrowPhaseData buffers[2];
    phyNarrowPhaseData* front = &buffers[0];
    phyNarrowPhaseData* back = &buffers[1];
    int tick_id = 0;

    phyManifold* findCachedManifold(manifold_key_t key);
    phyManifold* createNewManifold(manifold_key_t key);

public:
    int manifoldCount() const { return front->manifolds.count(); }
    phyManifold& getManifold(int at) { return front->manifolds[at]; }

    int oldManifoldCount() const { return back->manifolds.count(); }
    phyManifold& getOldManifold(int at) { return back->manifolds[at]; }

    void flipBuffers(int tick_id);

    phyManifold* getManifold(phyRigidBody* a, phyRigidBody* b, const gfxm::vec3& N, bool use_cached = true) {
        return getManifold(a, b, phyMakeNormalKey(N), N, use_cached);
    }
    phyManifold* getManifold(phyRigidBody* a, phyRigidBody* b, uint16_t normal_key, const gfxm::vec3& Ninitial = gfxm::vec3(), bool use_cached = true);

    void addContact(
        phyManifold* manifold,
        const gfxm::vec3& pt_a,
        const gfxm::vec3& pt_b,
        const gfxm::vec3& normal_a,
        const gfxm::vec3& normal_b,
        float distance
    );

    void primeManifold(phyManifold* manifold, phyContactPoint& cp);
    void removeManifold(int i);
    void rebuildManifold(phyManifold* m);

    void addContact3(phyManifold* m, phyContactPoint& cp);
    void addContact(phyManifold* manifold, phyContactPoint& cp);
};

