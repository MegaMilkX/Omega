cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0091 NEW)

project(OmegaEngine)

if (MSVC)
  add_compile_options(/bigobj)
else ()
  add_compile_options(-Wa,-mbig-obj)
endif ()

set(STARTUP_PROJECT_NAME "omega")
#set(BUILD_TOOLS_DIR "build_tools")
#set(DEBUG_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../runtime_data")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${STARTUP_PROJECT_NAME})

#file(MAKE_DIRECTORY ${DEBUG_WORKING_DIRECTORY})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

# common
add_subdirectory (
	./common
)

# omega
add_subdirectory (
	./main
)

# model_converter
add_subdirectory (
	./model_converter
)

# gui_test
add_subdirectory(
	./gui_test
)


# assimp
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ZLIB ON)
option(BUILD_SHARED_LIBS OFF)
add_subdirectory(
	./lib/assimp
	./lib/assimp/build
)

# freetype
add_subdirectory(
	./lib/freetype-2.10.0
	./lib/freetype-2.10.0/build
)

# fastnoisesimd
add_subdirectory(
	./lib/FastNoiseSIMD
	./lib/FastNoiseSIMD/build
)


