#pragma once

#include "mesh_base.auto.hpp"
#include "resource_manager/resource.hpp"
#include "resource_manager/resource_root.hpp"
#include "resource_manager/loadable.hpp"
#include "gpu/gpu_mesh_desc.hpp"


[[cppi_class]];
class Mesh
: public Resource
, public rtti::MetaObject
, public ILoadable
, public PolymorphicResourceRoot<Mesh> {
public:
    TYPE_ENABLE();

    // TODO: getMeshDesc must be pure virtual, default resource backend doesn't allow it to be
    virtual const gpuMeshDesc* getMeshDesc() const { return nullptr; };

    bool load(byte_reader&) override {
        return true;
    }
};

