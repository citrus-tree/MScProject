#version 450

float eps = 0.0001;
float pi = 3.141592;

float normal_bias = 0.02;

/* Here be data */

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;
layout(location = 2) in vec3 iNormal;

layout(location = 0) out vec4 oColour;

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 projection;
	mat4 projCam;
	vec4 position;
} cameraData;

layout(set = 1, binding = 0) uniform sampler2D uColourTex;
layout(set = 1, binding = 1) uniform sampler2D UMetallicRoughnessTex;

layout(set = 1, binding = 2) uniform MaterialData
{
	vec4 abledo;
	vec3 emissive;
	float roughness;
	vec3 transmission;
	float metallic;
	float _padding[4];
} uMaterialData;

struct DirectionalLight
{
	vec4 direction;
	vec4 colour;
};

struct AmbientLight
{
	vec4 colour;
};

layout(set = 2, binding = 0) uniform LightData
{
	DirectionalLight sunLight;
	AmbientLight ambientLight;
} lightingData;

layout(set = 3, binding = 0) uniform sampler2DShadow shadowMap;
layout(set = 3, binding = 1) uniform DirectionalShadowData
{
	mat4 view;
	mat4 projection;
	mat4 projView;
	mat4 cam2shadow;
} shadowData;

/* Helper functions */

float pos(float x)
{
	return max(0.0, x);
}

float posDot(vec3 left, vec3 right)
{
	float dot_val = dot(left, right);
	return max(0.0, dot_val);
}

/* Lighting and Shading Calculations */

vec3 LightingCalculation(vec3 position, vec3 normal, vec3 diffuse, float metallic, float roughness, vec3 cameraPosition)
{
	/* direct lighting strength componenets */ 
	vec3 to_cam = normalize(cameraPosition.rgb - position);
	vec3 to_light = normalize(-lightingData.sunLight.direction.rgb);
	vec3 half_vector = normalize(to_cam + to_light);

	/* shadow coverage calculation */
	vec3 normalBiasVector = normal * normal_bias;
	vec4 shadowViewPosition = shadowData.projView * vec4(position + normalBiasVector, 1.0);
	vec4 shadowCoords = vec4(shadowViewPosition / shadowViewPosition.w);
	shadowCoords.x = shadowCoords.x * 0.5 + 0.5;
	shadowCoords.y = shadowCoords.y * 0.5 + 0.5;
	shadowCoords.w = 1.0;

	float shadowStrength = textureProj(shadowMap, shadowCoords);

	vec3 direct = lightingData.sunLight.colour.rgb * diffuse * shadowStrength;
	vec3 ambient = lightingData.ambientLight.colour.rgb * diffuse;

	return ambient + (posDot(normal, to_light)) * direct;
}

/* main() */

void main()
{
	vec3 diffuse = texture(uColourTex, iUV).rgb;
	float metallic = uMaterialData.metallic;// * texture(UMetallicRoughnessTex, iUV).r;
	float roughness = uMaterialData.roughness;// * texture(UMetallicRoughnessTex, iUV).g;
	vec3 lit = uMaterialData.emissive + LightingCalculation(iPosition, normalize(iNormal), diffuse, 0.0, 0.0, cameraData.position.rgb);
	oColour = vec4(lit, 1.0);
}