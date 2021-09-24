#ifndef __TRIANGLE_H_
#define __TRIANGLE_H_

#include <string>

static const std::string triangleVertexShaderSrc = R"(

#version 430 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec4 a_AlternateColor;

uniform uint u_AlternateFlag;

out vec4 v_Color;

void main()
{
gl_Position = vec4(a_Position, 1.0f);

if(u_AlternateFlag != 1)
{
v_Color = vec4(a_Color,1.0f);
}
else
{
v_Color = a_AlternateColor;
}
}

)";

static const std::string triangleFragmentShaderSrc = R"(

#version 430 core

in vec4 v_Color;
out vec4 color;

void main()
{
color = v_Color;
}

)";

#endif // __TRIANGLE_H_
