cmake_minimum_required(VERSION 3.12)

project(Pacman)

find_package(OpenGL REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(glm)
add_subdirectory(glfw)
add_subdirectory(glad)

add_executable(Pacman
	main.cpp
	shaders/square.h)

target_link_libraries(Pacman
	PRIVATE
	glad
	glfw
	glm
	OpenGL::GL)
