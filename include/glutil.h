#ifndef GLUTIL_H
#define GLUTIL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <string>

/**
 * Initializes GLFW, creates a window that uses OpenGL 4.5 and loads GL functions using Glad
 */
GLFWwindow* initialize_glfw_and_gl();

/**
 * Reads everything from a single file
 */
std::string read_entire_file(const char* fp);

/**
 * @brief compile_shader compiles a shader from the provided shader source code
 * @param source is the source code of the shader
 * @param type is the type of the shader
 * @return OpenGL name of the newly created shader
 */
GLuint compile_shader(const char* source, GLenum type);

/**
 * @brief fcompile_shader compiles a shader from the given file
 * @param filepath is the path to the file which contains the shader source code
 * @param type is the type of the shader
 * @return OpenGL name of the newly created shader
 */
GLuint fcompile_shader(const char* filepath, GLenum type);

/**
 * @brief create_program will create and link a program from the two supplied shaders
 * @param shaders contains all the compiled shaders that should be linked in the program
 * @return OpenGL name of the newly created program
 */
GLuint create_program(const std::vector<GLuint>& shaders);

/**
 * Set up a debug callback with OpenGL
 */
void create_debug_callback();

/**
 * Get the byte size of a vector's contents
 *
 * @param vector The vector to get the byte size of
 * @return The size of the vector's contents in bytes
 */
template<typename T>
uint32_t vector_byte_size(const std::vector<T>& vector)
{
    return vector.size() * sizeof(T);
}

#endif //GLUTIL_H
