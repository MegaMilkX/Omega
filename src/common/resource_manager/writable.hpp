#pragma once

#include "byte_writer/byte_writer.hpp"


[[cppi_begin, no_reflect]];
class IWritable;
[[cppi_end]];

class IWritable {
public:
    virtual void write(byte_writer& out) const = 0;
};

