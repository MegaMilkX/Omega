#ifndef UTIL_WIN32_MODULE_HPP
#define UTIL_WIN32_MODULE_HPP

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

#include "string/split.hpp"

inline HMODULE win32GetThisModuleHandle()
{
  HMODULE h = NULL;
  GetModuleHandleExW(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCWSTR>(&win32GetThisModuleHandle),
    &h
  );
  return h;
}

inline std::string win32GetThisModuleName() {
    std::string filename;
    char buf[512];
    GetModuleFileNameA(win32GetThisModuleHandle(), buf, 512);
    filename = buf;
    auto tokens = strSplit(filename, '\\');
    return tokens[tokens.size() - 1];
}

#endif
