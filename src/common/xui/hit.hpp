#pragma once

#include <list>


namespace xui {


    enum class HIT {
        ERR = -1,
        NOWHERE,
        CLIENT,
        CAPTION,
        OUTSIDE_MENU,
    };

    class Element;

    struct Hit {
        HIT hit;
        Element* elem;
    };

    struct HitResult {
        std::list<Hit> hits;
        bool hasHit() const { 
            if (hits.empty()) {
                return false;
            }
            return hits.back().hit != HIT::NOWHERE && hits.back().hit != HIT::OUTSIDE_MENU;
        }
        void add(HIT type, Element* elem) {
            hits.push_back(Hit{ type, elem });
        }
        void clear() {
            hits.clear();
        }
    };


}

