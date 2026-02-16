cmake_minimum_required(VERSION 3.16)
project(scene-io VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SCENE_IO_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(scene-io STATIC ${SCENE_IO_SOURCES})

target_compile_features(scene-io PUBLIC cxx_std_23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(scene-io PUBLIC -std=gnu++2b)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(scene-io PUBLIC /std:c++latest)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../scene-core scene-core-build)

target_link_libraries(scene-io PUBLIC scene-core)

target_include_directories(scene-io
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:include>
)

set_target_properties(scene-io PROPERTIES OUTPUT_NAME "scene-io")

include(GNUInstallDirs)
install(TARGETS scene-io
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
