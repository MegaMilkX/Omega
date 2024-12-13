#pragma once

#include "csg_object.hpp"


class csgCustomShapeObject : public csgObject {
public:

    csgObject* makeCopy() const override {
        assert(false);
        return nullptr;
    }
};
