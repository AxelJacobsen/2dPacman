#ifndef __SQUARE_H_
#define __SQUARE_H_

#include <string>

static const std::string squareVertexShaderSrc = R"(
#version 430 core

layout(location = 0) in vec3 a_Position;

void main()
{
gl_Position = vec4(a_Position, 1.0f);
}
)";

static const std::string squareFragmentShaderSrc = R"(
#version 430 core

out vec4 color;

uniform vec4 u_Color;

void main()
{
color = u_Color;
}
)";


#endif // __SQUARE_H_
