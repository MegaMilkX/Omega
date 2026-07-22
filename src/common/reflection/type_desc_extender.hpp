#pragma once

#include "type_desc.hpp"


namespace rtti {


template<typename T, typename = void>
struct type_desc_extender {
	static void apply(type_desc&) {}
};


}