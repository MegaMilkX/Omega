#pragma once


enum ATTRIB_TYPE {
    ATTRIB_TOKENS,
    ATTRIB_STRING_LITERAL,
    ATTRIB_NUMBER
};

void set_attribute_type(const char* name, ATTRIB_TYPE type);
ATTRIB_TYPE get_attribute_type(const char* name);