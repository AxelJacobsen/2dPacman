cmake_minimum_required(VERSION 3.15)

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
	shaders/player.h
	shaders/map.h
	shaders/ghosts.h
	shaders/pellets.h 
	include/glutil.h)

target_link_libraries(Pacman
	PRIVATE
	glad
	glfw
	glm
	OpenGL::GL)

  target_include_directories(${PROJECT_NAME}
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/stb/include)
  
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  STB_IMAGE_IMPLEMENTATION)

  add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/assets/pacman.png
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/pacman.png)