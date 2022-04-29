cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0091 NEW)

set(PROJECT_NAME "omega")
set(BUILD_TOOLS_DIR "build_tools")
set(DEBUG_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../runtime_data")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

file(MAKE_DIRECTORY ${DEBUG_WORKING_DIRECTORY})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (${PROJECT_NAME})

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

# TODO: Fix. Files with h at the end not being processed
file(GLOB_RECURSE GENBINFILES ../embedded_data/*[!.h])
foreach(gen_bin_file ${GENBINFILES})
	get_filename_component(DIR ${gen_bin_file} DIRECTORY)
	get_filename_component(FNAME ${gen_bin_file} NAME)
	execute_process(
		COMMAND "../${BUILD_TOOLS_DIR}/bin2header/bin2header.exe" ${FNAME}
		WORKING_DIRECTORY ${DIR}
	)
	message(STATUS ${gen_bin_file})
endforeach(gen_bin_file)
# Move generated headers
file(GLOB_RECURSE GENBINHFILES ../embedded_data/*.h)
foreach(gen_header ${GENBINHFILES})
	get_filename_component(DIR ${gen_header} DIRECTORY)
	get_filename_component(FNAME ${gen_header} NAME)
	# get_filename_component(REL_FNAME ${gen_header} DIRECTORY BASE_DIR ../embedded_data/)
	
	message(STATUS ${DIR})
	message(STATUS ${FNAME})
	message(STATUS ${REL_FNAME})
	
	file(COPY ${gen_header} DESTINATION ../../src/engine/embedded_data/)
	file(REMOVE ${gen_header})
endforeach(gen_header)

file(GLOB_RECURSE SRCFILES 	
	RELATIVE ${CMAKE_SOURCE_DIR}
	./main/*.cpp;
	./main/*.c;
	./main/*.cxx;
	./main/*.h;
	./main/*.hpp;
)
message(STATUS "${SRCFILES}")
add_executable(${PROJECT_NAME} ${SRCFILES} ../build/resource.rc)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SRCFILES})

set_target_properties(
	${PROJECT_NAME} PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY ${DEBUG_WORKING_DIRECTORY}
#	VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../build/exec/release_with_debug_info"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../build/exec/release"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../build/exec/min_size_release"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../build/exec/debug"
)

target_include_directories(${PROJECT_NAME} PRIVATE 
	./main/
	./lib/
	./lib/assimp/include/
	./lib/freetype-2.10.0/include/
	./lib/FastNoiseSIMD/
)
target_link_directories(${PROJECT_NAME} PRIVATE 

)
target_link_libraries(${PROJECT_NAME} 
	shlwapi.lib
	OpenGL32.lib
	hid.lib
	setupapi.lib
	xaudio2.lib
	assimp
	freetype
	FastNoiseSIMD
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
	_CRT_SECURE_NO_WARNINGS
	NOMINMAX
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