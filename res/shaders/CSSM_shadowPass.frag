#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;

layout(location = 0) out vec4 oColour;

layout(set = 2, binding = 0) uniform sampler2D uNoiseTex;

void main()
{
	/* fragment depth */
	float depth = gl_FragCoord.z / gl_FragCoord.w;
	oColour = vec4(vec3(depth), 1.0);
}
