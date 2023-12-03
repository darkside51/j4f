#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;
layout (location = 0) out vec3 out_color;

layout(push_constant) uniform matrix {
	mat4 mvp;
} u_constants;

void main()  {
	gl_Position = u_constants.mvp * vec4(a_position, 1.0);
	out_color = a_color;
}