#version 460

const vec2 tri_verts[3] =
{
	vec2(-1, -1),
	vec2(-1, 3),
	vec2(3, -1)
};

const vec2 tri_uvs[3] =
{
	vec2(0, 0),
	vec2(0, 2),
	vec2(2, 0)
};

layout(location = 0) out vec2 oUV;

void main()
{
	oUV = tri_uvs[gl_VertexIndex];
	gl_Position = vec4(tri_verts[gl_VertexIndex], 0.5f, 1.0f);
}
