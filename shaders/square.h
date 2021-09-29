#ifndef __SQUARE_H_
#define __SQUARE_H_

#include <string>

static const std::string squareVertexShaderSrc = R"(
#version 430 core

layout(location = 0) in vec3 a_Position;

//We specify our uniforms. We do not need to specify locations manually, but it helps with knowing what is bound where.
layout(location=0) uniform mat4 u_TransformationMat = mat4(1);
layout(location=1) uniform mat4 u_ViewMat           = mat4(1);
layout(location=2) uniform mat4 u_ProjectionMat     = mat4(1);


void main()
{
//We multiply our matrices with our position to change the positions of vertices to their final destinations.
gl_Position = u_ProjectionMat * u_ViewMat * u_TransformationMat * vec4(a_Position, 1.0f);
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
