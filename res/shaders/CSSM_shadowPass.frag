#version 450

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

layout(set = 2, binding = 0) uniform sampler2D uNoiseTex;

void main()
{
	/* fragment depth */
	float depth = gl_FragCoord.z / gl_FragCoord.w;
	oColour = vec4(vec3(depth), 1.0);
}
