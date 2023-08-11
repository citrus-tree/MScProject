#version 450

float eps = 0.0001;
float pi = 3.141592;

float normal_bias = 0.035;

/* Here be data */

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;

layout(location = 0) out vec4 oColour;

layout(set = 1, binding = 0) uniform sampler2D uColourTex;
layout(set = 1, binding = 1) uniform sampler2D UMetallicRoughnessTex;

layout(set = 1, binding = 2) uniform MaterialData
{
	vec4 albedo;
	vec3 emissive;
	float roughness;
	vec3 transmission;
	float metallic;
	float _padding[4];
} uMaterialData;

/* main() */

void main()
{
	vec4 texSample = texture(uColourTex, iUV);
	vec3 lightProb = 0.5 + texSample.a * texSample.rgb * 0.5;

	oColour = vec4(lightProb, texSample.a);
}