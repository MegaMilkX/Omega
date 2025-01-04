#pragma once

#include "scene.auto.hpp"

#include "reflection/reflection.hpp"


#ifdef CPPI_PARSER
[[cppi_begin, no_reflect]];
class IScene;
[[cppi_end]];
#endif


class IScene {
public:
    TYPE_ENABLE();

    virtual ~IScene() = default;
};
