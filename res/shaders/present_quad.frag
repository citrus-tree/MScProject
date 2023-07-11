#version 450

layout(location = 0) in vec2 iUV;

layout(set = 0, binding = 0) uniform sampler2D uIntermediate;

layout(location = 0) out vec4 oColour;

void main()
{
	oColour = vec4(texture(uIntermediate, iUV).rgb, 1.0);
}
