#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 0) out vec2 out_uv;

layout(push_constant) uniform matrix {
	mat4 mvp;
} u_constants;

void main()  {
	gl_Position = u_constants.mvp * vec4(a_position, 1.0);
	out_uv = a_uv;
}