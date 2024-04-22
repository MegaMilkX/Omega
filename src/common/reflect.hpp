#pragma once

#include <assert.h>
#include "reflection/reflection.hpp"

template<typename T>
void reflect() {
	assert(false);
}

#define REFLECT(TYPE) \
template<> inline void reflect<TYPE>()



void reflectInit();