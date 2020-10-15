cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
	
set(EXE_TARGET_NAME Test)
	
file (GLOB SRC_FILES ./*.cpp)

add_executable(${EXE_TARGET_NAME} ${SRC_FILES})

target_link_libraries(${EXE_TARGET_NAME} 
	PRIVATE
		ZppLibrary
		)
