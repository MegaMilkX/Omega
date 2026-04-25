cmake_minimum_required (VERSION 3.12)
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

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${STARTUP_PROJECT_NAME})

set(LIB_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

# Disable INSTALL and PACKAGE targets
set(CMAKE_SKIP_INSTALL_RULES 1)

# common
add_subdirectory (
	./common
)

# omega
add_subdirectory (
	./main
)

# gui_test
add_subdirectory(
	./gui_test
)

# gui_test2
add_subdirectory(
	./gui_test2
)


# assimp
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ZLIB ON)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(
	./lib/assimp
)
set_target_properties(assimp	PROPERTIES FOLDER "lib")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs	PROPERTIES FOLDER "lib")
set_target_properties(zlibstatic	PROPERTIES FOLDER "lib")
set_target_properties(assimp 	PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
)


# freetype
add_subdirectory(
	./lib/freetype-2.10.0
)
set_target_properties(freetype		PROPERTIES FOLDER "lib")
set_target_properties(freetype 		PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
)


# fastnoisesimd
add_subdirectory(
	./lib/FastNoiseSIMD
)
set_target_properties(FastNoiseSIMD		PROPERTIES FOLDER "lib")
set_target_properties(FastNoiseSIMD 	PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${LIB_OUTPUT_DIR}/$<CONFIG>"
)

