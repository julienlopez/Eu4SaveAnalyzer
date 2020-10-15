cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(LIBRARY_TARGET_NAME ZppLibrary)

set(LIB_ROOT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
set(LIB_ALL_SRC Zpp.cpp Util.cpp izstream.cpp)
file(GLOB_RECURSE LIB_ALL_HDR "${LIB_ROOT_DIRECTORY}/*.hpp")

include_directories(${LIB_ROOT_DIRECTORY})
	
add_library(${LIBRARY_TARGET_NAME} ${LIB_ALL_SRC} ${LIB_ALL_HDR})
														
set_target_properties(${LIBRARY_TARGET_NAME} PROPERTIES PUBLIC_HEADER "${LIB_ALL_HDR}")														

target_include_directories(${LIBRARY_TARGET_NAME} INTERFACE "$<BUILD_INTERFACE:${LIB_ROOT_DIRECTORY}>"
                                                         "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")
														 
find_package(Zlib REQUIRED)
target_link_libraries(${LIBRARY_TARGET_NAME} PUBLIC ZLIB::ZLIB)
	
message(STATUS "Created target ${LIBRARY_TARGET_NAME} for export ${PROJECT_NAME}.")
