#include "gjk.hpp"

#include "debug_draw/debug_draw.hpp"


GJK_SupportPoint GJK_supportMinkowski(const SphereShapeGJK& A, const SphereShapeGJK& B, const gfxm::vec3& dir) {
    GJK_SupportPoint sp;
    sp.A = A.support(dir);
    sp.B = B.support(-dir);
    sp.M = sp.A - sp.B;
    return sp;
}

gfxm::vec3 GJK_tripleCross(const gfxm::vec3& a, const gfxm::vec3& b, const gfxm::vec3& c) {
    return gfxm::cross(a, gfxm::cross(b, c));
}

bool GJK_containsOrigin(GJK_Simplex& simplex, gfxm::vec3& dir) {
    if (simplex.count == 2) {
        gfxm::vec3 A = simplex[1].M;
        gfxm::vec3 B = simplex[0].M;
        gfxm::vec3 AB = B - A;
        gfxm::vec3 AO = -A;

        dir = GJK_tripleCross(AB, AO, AB);
        if(gfxm::length(dir) < 1e-6f) {
            dir = gfxm::cross(AB, gfxm::vec3(1, 0, 0));
            if(gfxm::length(dir) < 1e-6f) {
                dir = gfxm::cross(AB, gfxm::vec3(0, 1, 0));
            }
        }
        return false;
    } else if(simplex.count == 3) {
        gfxm::vec3 A = simplex[2].M;
        gfxm::vec3 B = simplex[1].M;
        gfxm::vec3 C = simplex[0].M;
        gfxm::vec3 AB = B - A;
        gfxm::vec3 AC = C - A;
        gfxm::vec3 AO = -A;

        gfxm::vec3 ABC = gfxm::cross(AB, AC);

        gfxm::vec3 ABperp = gfxm::cross(AB, ABC);
        if (gfxm::dot(ABperp, AO) > .0f) {
            simplex.remove_point(0);
            dir = GJK_tripleCross(AB, AO, AB);
            return false;
        }

        gfxm::vec3 ACperp = gfxm::cross(ABC, AC);
        if (gfxm::dot(ACperp, AO) > .0f) {
            simplex.remove_point(1);
            dir = GJK_tripleCross(AC, AO, AC);
            return false;
        }

        if (gfxm::dot(ABC, AO) > .0f) {
            dir = ABC;
        } else {
            std::swap(simplex[0], simplex[1]);
            dir = -ABC;
        }

        return false;
    } else if(simplex.count == 4) {
        gfxm::vec3 A = simplex[3].M;
        gfxm::vec3 B = simplex[2].M;
        gfxm::vec3 C = simplex[1].M;
        gfxm::vec3 D = simplex[0].M;
        gfxm::vec3 AO = -A;

        gfxm::vec3 ABC = gfxm::cross(B - A, C - A);
        gfxm::vec3 ACD = gfxm::cross(C - A, D - A);
        gfxm::vec3 ADB = gfxm::cross(D - A, B - A);

        if (gfxm::dot(ABC, AO) > 0) {
            simplex.remove_point(0);
            dir = ABC;
            return false;
        }
        if (gfxm::dot(ACD, AO) > 0) {
            simplex.remove_point(2);
            dir = ACD;
            return false;
        }
        if (gfxm::dot(ADB, AO) > 0) {
            simplex.remove_point(1);
            dir = ADB;
            return false;
        }
        return true;
    }
    return false;
}

void GJK_debugDrawSimplex(const gfxm::vec3& at, const GJK_Simplex& simplex) {
    if (simplex.count == 1) {
        gfxm::vec3 A = simplex[0].M;
        dbgDrawSphere(at + A, .05f, 0xFF0000FF);
    } else if (simplex.count == 2) {
        gfxm::vec3 A = simplex[1].M;
        gfxm::vec3 B = simplex[0].M;
        dbgDrawLine(at + A, at + B, 0xFF00FFFF);        
    } else if(simplex.count == 3) {
        gfxm::vec3 A = simplex[2].M;
        gfxm::vec3 B = simplex[1].M;
        gfxm::vec3 C = simplex[0].M;
        dbgDrawLine(at + A, at + B, 0xFFFF00FF);
        dbgDrawLine(at + B, at + C, 0xFFFF00FF);
        dbgDrawLine(at + C, at + A, 0xFFFF00FF);
        dbgDrawSphere(at + A, .05f, 0xFF0000FF);
        dbgDrawSphere(at + B, .05f, 0xFF00FF00);
        dbgDrawSphere(at + C, .05f, 0xFFFF0000);
    } else if(simplex.count == 4) {
        gfxm::vec3 A = simplex[3].M;
        gfxm::vec3 B = simplex[2].M;
        gfxm::vec3 C = simplex[1].M;
        gfxm::vec3 D = simplex[0].M;
        dbgDrawLine(at + A, at + B, 0xFFFFFF00);
        dbgDrawLine(at + B, at + C, 0xFFFFFF00);
        dbgDrawLine(at + C, at + A, 0xFFFFFF00);

        dbgDrawLine(at + A, at + B, 0xFFFFFF00);
        dbgDrawLine(at + B, at + D, 0xFFFFFF00);
        dbgDrawLine(at + D, at + A, 0xFFFFFF00);

        dbgDrawLine(at + B, at + C, 0xFFFFFF00);
        dbgDrawLine(at + C, at + D, 0xFFFFFF00);
        dbgDrawLine(at + D, at + B, 0xFFFFFF00);

        dbgDrawLine(at + C, at + A, 0xFFFFFF00);
        dbgDrawLine(at + A, at + D, 0xFFFFFF00);
        dbgDrawLine(at + D, at + C, 0xFFFFFF00);
    }
}
void GJK_debugDrawSimplexA(const gfxm::vec3& at, const GJK_Simplex& simplex) {
    if (simplex.count == 1) {
        gfxm::vec3 A = simplex[0].A;
        dbgDrawSphere(A, .05f, 0xFF0000FF);
    } else if (simplex.count == 2) {
        gfxm::vec3 A = simplex[1].A;
        gfxm::vec3 B = simplex[0].A;
        dbgDrawLine(A, B, 0xFF00FFFF);        
    } else if(simplex.count == 3) {
        gfxm::vec3 A = simplex[2].A;
        gfxm::vec3 B = simplex[1].A;
        gfxm::vec3 C = simplex[0].A;
        dbgDrawLine(A, B, 0xFFFF00FF);
        dbgDrawLine(B, C, 0xFFFF00FF);
        dbgDrawLine(C, A, 0xFFFF00FF);
        dbgDrawSphere(A, .05f, 0xFF0000FF);
        dbgDrawSphere(B, .05f, 0xFF00FF00);
        dbgDrawSphere(C, .05f, 0xFFFF0000);
    } else if(simplex.count == 4) {
        gfxm::vec3 A = simplex[3].A;
        gfxm::vec3 B = simplex[2].A;
        gfxm::vec3 C = simplex[1].A;
        gfxm::vec3 D = simplex[0].A;
        dbgDrawLine(A, B, 0xFFFFFF00);
        dbgDrawLine(B, C, 0xFFFFFF00);
        dbgDrawLine(C, A, 0xFFFFFF00);

        dbgDrawLine(A, B, 0xFFFFFF00);
        dbgDrawLine(B, D, 0xFFFFFF00);
        dbgDrawLine(D, A, 0xFFFFFF00);

        dbgDrawLine(B, C, 0xFFFFFF00);
        dbgDrawLine(C, D, 0xFFFFFF00);
        dbgDrawLine(D, B, 0xFFFFFF00);

        dbgDrawLine(C, A, 0xFFFFFF00);
        dbgDrawLine(A, D, 0xFFFFFF00);
        dbgDrawLine(D, C, 0xFFFFFF00);
    }
}
void GJK_debugDrawSimplexB(const gfxm::vec3& at, const GJK_Simplex& simplex) {
    if (simplex.count == 1) {
        gfxm::vec3 A = simplex[0].B;
        dbgDrawSphere(A, .05f, 0xFF0000FF);
    } else if (simplex.count == 2) {
        gfxm::vec3 A = simplex[1].B;
        gfxm::vec3 B = simplex[0].B;
        dbgDrawLine(A, B, 0xFF00FFFF);        
    } else if(simplex.count == 3) {
        gfxm::vec3 A = simplex[2].B;
        gfxm::vec3 B = simplex[1].B;
        gfxm::vec3 C = simplex[0].B;
        dbgDrawLine(A, B, 0xFFFF00FF);
        dbgDrawLine(B, C, 0xFFFF00FF);
        dbgDrawLine(C, A, 0xFFFF00FF);
        dbgDrawSphere(A, .05f, 0xFF0000FF);
        dbgDrawSphere(B, .05f, 0xFF00FF00);
        dbgDrawSphere(C, .05f, 0xFFFF0000);
    } else if(simplex.count == 4) {
        gfxm::vec3 A = simplex[3].B;
        gfxm::vec3 B = simplex[2].B;
        gfxm::vec3 C = simplex[1].B;
        gfxm::vec3 D = simplex[0].B;
        dbgDrawLine(A, B, 0xFFFFFF00);
        dbgDrawLine(B, C, 0xFFFFFF00);
        dbgDrawLine(C, A, 0xFFFFFF00);

        dbgDrawLine(A, B, 0xFFFFFF00);
        dbgDrawLine(B, D, 0xFFFFFF00);
        dbgDrawLine(D, A, 0xFFFFFF00);

        dbgDrawLine(B, C, 0xFFFFFF00);
        dbgDrawLine(C, D, 0xFFFFFF00);
        dbgDrawLine(D, B, 0xFFFFFF00);

        dbgDrawLine(C, A, 0xFFFFFF00);
        dbgDrawLine(A, D, 0xFFFFFF00);
        dbgDrawLine(D, C, 0xFFFFFF00);
    }
}

bool GJK(const SphereShapeGJK& A, const SphereShapeGJK& B, GJK_Simplex& simplex) {
    gfxm::vec3 dir(0, 1, 0);
    dir = B.pos - A.pos;

    GJK_SupportPoint sp = GJK_supportMinkowski(A, B, dir);
    simplex.insert_point(sp);

    dir = -sp.M; // Towards the origin

    for(int i = 0; i < GJK_MAX_ITERATIONS; ++i) {
        GJK_SupportPoint np = GJK_supportMinkowski(A, B, dir);
        if (gfxm::dot(np.M, dir) <= .0f) {
            return false;
        }


        simplex.insert_point(np);
        
        //GJK_debugDrawSimplex(simplex);
        if(GJK_containsOrigin(simplex, dir)) {
            return true;
        }
    }

    // Ran out of iterations
    return false;
}

