#ifndef UTIL_WIN32_MODULE_HPP
#define UTIL_WIN32_MODULE_HPP

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

#include <util/string/split.hpp>

inline HMODULE this_module_handle()
{
  HMODULE h = NULL;
  GetModuleHandleExW(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCWSTR>(&this_module_handle),
    &h
  );
  return h;
}

inline std::string this_module_name() {
    std::string filename;
    char buf[512];
    GetModuleFileNameA(this_module_handle(), buf, 512);
    filename = buf;
    auto tokens = split(filename, '\\');
    return tokens[tokens.size() - 1];
}

#endif
