#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

layout (location = 0) out vec2 uv;
layout (location = 1) out float viewDepth;
layout (location = 2) out vec3 position;

layout (set = 0, binding = 0) uniform static_UBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	float lightMin;
	vec4 lightColor;
	mat4 view;
} u_constants;

layout(push_constant) uniform matrix {
	mat4 mvp;
} pushConst;

void main()  {
	vec3 viewPos = (u_constants.view * vec4(a_position, 1.0)).xyz; // u_constants.view * model * vec4(a_position, 1.0);
	gl_Position = pushConst.mvp * vec4(a_position, 1.0);

	uv = a_uv;
	viewDepth = viewPos.z;
	position = a_position; // model * vec4(a_position, 1.0);
}