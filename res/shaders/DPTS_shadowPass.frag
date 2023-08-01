#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;

layout(set = 1, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 projection;
	mat4 projView;
	vec4 position;
} cameraData;

layout(set = 2, binding = 0) uniform sampler2D coolerDepthMap;

void main()
{
	cameraPosition
	
	/* simulate a second greater than depth test */
	float depth = gl_FragCoord.z / gl_FragCoord.w;
	if (texture(coolerDepthMap, gl_FragCoord.rg / gl_FragCoord.w).r >= depth)
		discard;
}
