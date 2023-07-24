#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;

layout(location = 0) out vec3 oPosition;
layout(location = 1) out vec2 oUV;

layout(set = 0, binding = 0) uniform ShadowData
{
	mat4 view;
	mat4 projection;
	mat4 projView;
} shadowData;

void main()
{
	oPosition = iPosition;
	oUV = iUV;

	gl_Position = shadowData.projView * vec4(iPosition, 1.0f);
}