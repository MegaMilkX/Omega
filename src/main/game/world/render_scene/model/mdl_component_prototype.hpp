#pragma once

#include "mdl_component_mutable.hpp"

#include "reflection/reflection.hpp"

class mdlModelPrototype;
struct mdlComponentPrototype {
    TYPE_ENABLE_BASE();

    virtual ~mdlComponentPrototype() {}

    virtual void make(mdlModelPrototype* owner, mdlComponentMutable** m, int count) = 0;

    virtual void reflect() = 0;
};