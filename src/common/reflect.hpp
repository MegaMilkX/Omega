#pragma once

#include "reflection/reflection.hpp"

template<typename T>
void reflect() { static_assert(false, "reflect() for type T not implemented"); }

#define REFLECT(TYPE) \
template<> inline void reflect<TYPE>()



void reflectInit();