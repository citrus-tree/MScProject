#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;

layout(set = 0, binding = 0) uniform ShadowData
{
	mat4 view;
	mat4 projection;
	mat4 projView;
	mat4 invView;
} shadowData;

void main()
{
	gl_Position = shadowData.projView * vec4(iPosition, 1.0f);
}