#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iUV;
layout(location = 2) in vec3 iNormal;

layout(location = 0) out vec3 oPosition;
layout(location = 1) out vec2 oUV;
layout(location = 2) out vec3 oNormal;
layout(location = 3) out vec4 oTangent;
layout(location = 4) out vec3 oBitangent;

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 projection;
	mat4 projView;
	vec4 position;
} cameraData;

void main()
{
	oPosition = iPosition;
	oUV = iUV;
	oNormal = iNormal;

	gl_Position = cameraData.projView * vec4(iPosition, 1.0f);
}