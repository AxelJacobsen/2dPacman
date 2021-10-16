#ifndef __GHOSTS_H_
#define __GHOSTS_H_

#include <string>
#include <vector>
static const std::string ghostVertexShaderSrc = R"(
#version 430 core

/** Outputs */
out vec2 vsTexcoord;

/** Locations */
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 gTexcoord;

//We specify our uniforms. We do not need to specify locations manually, but it helps with knowing what is bound where.
layout(location=0) uniform mat4 u_TransformationMat = mat4(1);


void main()
{
vsTexcoord = gTexcoord;
//We multiply our matrices with our position to change the positions of vertices to their final destinations.
gl_Position = u_TransformationMat * vec4(a_Position, 1.0f);
}
)";

static const std::string ghostFragmentShaderSrc = R"(
#version 430 core

/** Inputs */
in vec2 vsTexcoord;

/** Outputs */
out vec4 color;

uniform vec4 gColor;
uniform sampler2D g_GhostTexture;

void main()
{
	vec4 textColorG = texture(g_GhostTexture, vsTexcoord);
	if(textColorG.a < 0.1)
        discard;
	color = textColorG;
}
)";


#endif // __GHOSTS_H_