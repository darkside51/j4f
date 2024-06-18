#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out float out_viewDepth;
layout (location = 2) out vec3 out_position;

layout (set = 0, binding = 0) uniform static_shadow_UBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
} u_shadow;

layout(push_constant) uniform matrix {
	mat4 mvp;
} u_constants;

void main()  {
	vec3 viewPos = (u_shadow.view * vec4(a_position, 1.0)).xyz;
	gl_Position = u_constants.mvp * vec4(a_position, 1.0);
	out_uv = a_uv;
	out_viewDepth = viewPos.z;
	out_position = a_position;
}