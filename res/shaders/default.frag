#version 450

float eps = 0.0001;
float pi = 3.141592;

/* Here be data */

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec4 iTangent;
layout(location = 4) in vec3 iBitangent;

layout(location = 0) out vec4 oColour;

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 projection;
	mat4 projCam;
	vec4 position;
} cameraData;

layout(set = 1, binding = 0) uniform sampler2D uColourTex;
layout(set = 1, binding = 1) uniform sampler2D uRoughnessTex;
layout(set = 1, binding = 2) uniform sampler2D uMetallicTex;

/* main() */

void main()
{
	vec4 diffuse = texture(uColourTex, iUV);
	oColour = diffuse;
}