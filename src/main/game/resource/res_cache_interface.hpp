#pragma once


class resCacheInterface {
public:
    virtual void* get(const char* name) = 0;
};