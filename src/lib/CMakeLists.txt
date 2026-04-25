cmake_minimum_required (VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(Libraries)

if (MSVC)
  add_compile_options(/bigobj)
else ()
  add_compile_options(-Wa,-mbig-obj)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

set(LIB_OUTPUT_DIR "${CMAKE_BINARY_DIR}/../lib")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

# Disable INSTALL and PACKAGE targets
set(CMAKE_SKIP_INSTALL_RULES 1)

# assimp
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ZLIB ON)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(
	./assimp
)

# freetype
add_subdirectory(
	./freetype-2.10.0
)

# fastnoisesimd
add_subdirectory(
	./FastNoiseSIMD
)


set_target_properties(assimp PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # static
	LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # shared
	RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # exec
)
set_target_properties(zlibstatic PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # static
	LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # shared
	RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # exec
)
set_target_properties(freetype PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # static
	LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # shared
	RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # exec
)
set_target_properties(FastNoiseSIMD PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # static
	LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # shared
	RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>" # exec
)