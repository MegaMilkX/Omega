
// Forward declarations
// Used for the parser to recognize types that it does not need to generate reflection code for

// TODO: An auto file should not be generated for this file
// TODO: This file should not be included in the unity cpp

#if defined(CPPI_PARSER)

[[cppi_begin, no_reflect]];


namespace std {
template<typename T>
class shared_ptr;
template<typename T>
class unique_ptr;

class string;

template<typename T>
class vector;
}

namespace gfxm {
struct vec2;
struct vec3;
struct vec4;
}

namespace nlohmann {
class json;
}

template<typename T>
class curve;

template<typename T>
class RHSHARED;

[[cppi_end]];

#endif
