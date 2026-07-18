#pragma once

#include <type_traits>


using type_id_t = uint32_t;

template<typename T>
using unqualified_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

