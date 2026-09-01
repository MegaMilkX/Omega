#pragma once

#include "resource_manager/resource_ref.hpp"
#include "collider_resource_backend.hpp"
#include "resource_manager/loadable.hpp"
#include "collision/shape/shape.hpp"


class Collider;
RESOURCE_BACKEND(Collider, ColliderResourceBackend);

class Collider
    : public ILoadable
    , public PolymorphicResourceRoot<Collider> {
public:
    virtual ~Collider() {}
    virtual phyShape* getShape() const { return nullptr; }

    // TODO: This should not be necessary since we don't use BasicResourceBackend for colliders
    DEFINE_EXTENSIONS(e_shp);
    bool load(byte_reader& in) { return false; }
};