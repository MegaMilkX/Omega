set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(FastNoiseSIMD STATIC 
	FastNoiseSIMD.cpp
	FastNoiseSIMD_avx2.cpp
	FastNoiseSIMD_avx512.cpp
	FastNoiseSIMD_internal.cpp
	FastNoiseSIMD_neon.cpp
	FastNoiseSIMD_sse2.cpp
	FastNoiseSIMD_sse41.cpp
)