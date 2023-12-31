#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;

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
	/* perhaps make the method of sampling the noise better */
	vec2 samplingUV = vec2(iPosition.x + iPosition.y + iUV.x, iPosition.z - iPosition.y + iUV.y);
	samplingUV.x = samplingUV.x - floor(samplingUV.x) * 1000.0;
	samplingUV.y = samplingUV.y - floor(samplingUV.y) * 1000.0;
	float random = textureLod(uNoiseTex, samplingUV, 0).r;

	/* calculate the fragment alpha */
	float alpha = texture(uColourTex, iUV).a;

	/* TODO: fix this implementation to use stratified sampling */
	/* discard fragments that do not pass */
	if (alpha < random)
		discard;
}
