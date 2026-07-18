#pragma once

#include "actor_component.auto.hpp"
#include "reflection/reflection.hpp"

[[cppi_class]];
class ActorComponent : public rtti::MetaObject {
public:
    TYPE_ENABLE();

    virtual ~ActorComponent() {}
};