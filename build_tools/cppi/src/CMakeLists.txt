cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0091 NEW)

set(PROJECT_NAME "cppi")
set(DEBUG_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../runtime")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

file(MAKE_DIRECTORY ${DEBUG_WORKING_DIRECTORY})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (${PROJECT_NAME})

if(MSVC)
  add_definitions(/MP)
  #add_definitions(/incremental)
  #add_definitions(/Debug:fastlink)
endif()

file(GLOB_RECURSE SRCFILES 	
	RELATIVE ${PROJECT_SOURCE_DIR}
	./*.cpp;
	./*.c;
	./*.cxx;
	./*.h;
	./*.hpp;
)
add_executable(${PROJECT_NAME} ${SRCFILES})
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SRCFILES})

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
	./
	./lib
)
target_link_directories(${PROJECT_NAME} PRIVATE 

)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
	_CRT_SECURE_NO_WARNINGS
	NOMINMAX
)
