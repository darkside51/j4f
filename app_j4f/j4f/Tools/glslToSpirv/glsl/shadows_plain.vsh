#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

layout (location = 0) out vec2 uv;
layout (location = 1) out float viewDepth;
layout (location = 2) out vec3 position;

layout(push_constant) uniform matrix {
	mat4 mvp;
	mat4 view;
} pushConst;

void main()  {
	vec3 viewPos = (pushConst.view * vec4(a_position, 1.0)).xyz; // u_ubo.view * model * vec4(a_position, 1.0);
	viewDepth = viewPos.z;

	gl_Position = pushConst.mvp * vec4(a_position, 1.0);
	uv = a_uv;
	position = a_position;//model * vec4(a_position, 1.0);
}