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
	/* perhaps make the method of sampling the noise better */
	vec2 samplingUV = vec2(iPosition.x + iPosition.y + iUV.x, iPosition.z - iPosition.y + iUV.y);
	samplingUV.x = samplingUV.x - floor(samplingUV.x) * 1000.0;
	samplingUV.y = samplingUV.y - floor(samplingUV.y) * 1000.0;
	vec3 random = textureLod(uNoiseTex, samplingUV, 0).rgb;

	/* calculate the fragment alpha */
	vec4 texSample = texture(uColourTex, iUV);
	float alpha = texSample.a;

	/* for the purposes of this project's implementation transmission is assumed to be the same as diffuse colour */
	vec3 transmission =  texSample.rgb * 0.5;
	vec3 lightProb = alpha * (1.0 - transmission);

	/* fragment depth */
	float depth = gl_FragCoord.z / gl_FragCoord.w;

	/* light colour */
	oColour.r = max(depth, float(random.r > lightProb.r));
	oColour.g = max(depth, float(random.g > lightProb.g));
	oColour.b = max(depth, float(random.b > lightProb.b));
	oColour.a = 1.0;
}
