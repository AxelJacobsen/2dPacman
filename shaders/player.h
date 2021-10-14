#ifndef __PLAYER_H_
#define __PLAYER_H_

#include <string>

static const std::string playerVertexShaderSrc = R"(
#version 430 core

/** Inputs */
in vec2 aTexcoord;

/** Outputs */
out vec3 vsColor;
out vec2 vsTexcoord;

layout(location = 0) in vec3 a_Position;

//We specify our uniforms. We do not need to specify locations manually, but it helps with knowing what is bound where.
layout(location=0) uniform mat4 u_TransformationMat = mat4(1);
layout(location=1) uniform mat4 u_ViewMat           = mat4(1);
layout(location=2) uniform mat4 aTexcoord           = mat4(1);


void main()
{
//Pass the color and texture data for the fragment shader
vsColor    = aColor;
vsTexcoord = aTexcoord;
//We multiply our matrices with our position to change the positions of vertices to their final destinations.
gl_Position = u_ViewMat * u_TransformationMat * vec4(a_Position, 1.0f);
}
)";

static const std::string playerFragmentShaderSrc = R"(
#version 430 core

/** Inputs */
in vec3 vsColor;
in vec2 vsTexcoord;

/** Outputs */
out vec4 outColor;

/** Binding specifies what texture slot the texture should be at (in this case TEXTURE0) */
uniform sampler2D u_PlayerTexture;

void main()
{
	vec4 PacmanText = texture(uTextureA, vsTexcoord);
	outColor = PacmanText * vec4(vsColor, 1.0);
}
)";


#endif // __PLAYER_H_
