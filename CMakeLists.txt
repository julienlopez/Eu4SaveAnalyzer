cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

project(Eu4SaveAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
	add_definitions("-fprofile-arcs")
	add_definitions("-ftest-coverage")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage  -lstdc++fs")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) 
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_SHARED_LIBS "Build libraries as shared as opposed to static" OFF)

# include_directories(3rd_party/GSL/include)
# include_directories(3rd_party/expected/include)

add_subdirectory(3rd_party/zpp-1.0-alpha)

