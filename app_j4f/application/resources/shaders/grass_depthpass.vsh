#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_joints;
layout (location = 3) in vec4 a_weights;
layout (location = 4) in vec2 a_uv;

layout (set = 0, binding = 0) uniform static_lightUBO {
	vec3 lightDirection;
	vec2 lightMinMax;
	vec4 lightColor;
} u_constants;

layout (set = 2, binding = 0) readonly buffer static_SSBO {
	mat4 models[10000];
} u_transforms;

layout (set = 3, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
} u_shadow;

layout (set = 5, binding = 0) uniform UBO {
	int use_skin;
	mat4 skin_matrixes[192];
} u_ubo;

layout(push_constant) uniform PUSH_CONST {
	mat4 camera_matrix;
	mat4 model_matrix;
} u_push_const;

out gl_PerVertex {
    vec4 gl_Position;   
};

layout (location = 0) out vec2 out_uv;

void main() {
	out_uv = a_uv;
	mat4 modelMatrix = u_transforms.models[gl_InstanceIndex];
	gl_Position = u_push_const.camera_matrix * modelMatrix * vec4(a_position, 1.0);
}