cmake_minimum_required(VERSION 3.0)

project(
	Procrastinase
	LANGUAGES CXX
)

# enable C++11 support in compiler 
if (CMAKE_VERSION VERSION_LESS "3.1")
	if (
		(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	)
		set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
	endif ()
else ()
	set (CMAKE_CXX_STANDARD 11)
endif ()

# enable full warnings
if (
	(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
)
	set (CMAKE_CXX_FLAGS "-Wall -pedantic ${CMAKE_CXX_FLAGS}")
endif ()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	message(FATAL_ERROR "Win32 interface isn't yet implemented")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	message(FATAL_ERROR "Cocoa interface isn't yet implemented")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux") # there's no portable pid_t -> executable path mapping >:(
	find_library(XCB_LIB xcb)
	find_path(XCB_H xcb/xcb.h)
	if (
		(NOT ${XCB_H} STREQUAL "XCB_H_NOTFOUND")
		AND (NOT ${XCB_LIB} STREQUAL "XCB_LIB_NOTFOUND")
	)
		include_directories(${XCB_H})
		add_library(window_interface "src/foreign_window_xcb.cpp" "src/window_watcher_xcb.cpp")
		target_link_libraries(window_interface ${XCB_LIB})
	else()
		message(FATAL_ERROR "XCB can't be found")
	endif()
else()
	message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} isn't supported (yet?)")
endif()

set(PROCRASTINASE_SOURCES "src/window_watcher.cpp")

add_executable(skeleton "src/skeleton.cpp" ${PROCRASTINASE_SOURCES})
target_link_libraries(skeleton window_interface)
