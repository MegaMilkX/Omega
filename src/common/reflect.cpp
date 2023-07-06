#include "reflect.hpp"


// NOTE: there's no reflectCleanup() on purpose
void reflectInit() {
    // NOTE: not sure, maybe should call reflect<T>() in appropriate systems init() functions?
    // otherwise we are reflecting types all over, even when we don't need them
    LOG_ERR("TODO: Please change all type registration to reflect<T>() calls in their appropriate systems")
}
