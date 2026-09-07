#pragma once

#include "collider.auto.hpp"
#include "resource_manager/resource_ref.hpp"
#include "collider_resource_backend.hpp"
#include "resource_manager/loadable.hpp"
#include "collision/shape/shape.hpp"


class Collider;
RESOURCE_BACKEND(Collider, ColliderResourceBackend);

[[cppi_class]];
class Collider
    : public ILoadable
    , public PolymorphicResourceRoot<Collider> {
public:
    TYPE_ENABLE();

    virtual ~Collider() {}
    virtual phyShape* getShape() const { return nullptr; }

    // TODO: This should not be necessary since we don't use BasicResourceBackend for colliders
    DEFINE_EXTENSIONS(e_shp);
    bool load(byte_reader& in) { return false; }
};