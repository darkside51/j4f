#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_uv;

layout(push_constant) uniform PUSH_CONST {
	mat4 camera_matrix;
	mat4 model_matrix;
} u_push_const;

layout (location = 0) out vec3 out_uv;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	out_uv = a_uv;
	mat4 m = u_push_const.camera_matrix;
	m[3].xyzw = vec4(0.0, 0.0, 0.0, 1.0);
	gl_Position = m * (u_push_const.model_matrix * vec4(a_position, 1.0));
}