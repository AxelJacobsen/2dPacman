#ifndef __PLAYER_H_
#define __PLAYER_H_

#include <string>

static const std::string playerVertexShaderSrc = R"(
#version 430 core

/** Inputs */
in vec3 pPosition;
in vec2 pTexcoord;

/** Outputs */
out vec2 vsTexcoord;

/** Uniform */
layout(location=0) uniform mat4 u_TransformationMat = mat4(1);

void main()
{
//Pass the color and texture data for the fragment shader
vsTexcoord = pTexcoord;
//We multiply our matrices with our position to change the positions of vertices to their final destinations.
gl_Position =  u_TransformationMat * vec4(pPosition, 1.0f);
}
)";

static const std::string playerFragmentShaderSrc = R"(
#version 430 core

/** Inputs */
in vec2 vsTexcoord;

/** Outputs */
out vec4 color;

/** Binding specifies what texture slot the texture should be at (in this case TEXTURE0) */
uniform sampler2D u_PlayerTexture;

void main()
{
	vec4 textColorP = texture(u_PlayerTexture, vsTexcoord);
	if(textColorP.a < 0.1)
        discard;
	color = textColorP;
}
)";

#endif // __PLAYER_H_
