#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

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
	float u_time;
} u_ubo;

layout(push_constant) uniform PUSH_CONST {
	mat4 camera_matrix;
	mat4 model_matrix;
} u_push_const;

layout (location = 0) out vec3 out_normal;
layout (location = 1) out vec2 out_uv;

layout (location = 2) out float out_view_depth;
layout (location = 3) out vec3 out_position;

layout (location = 4) out float out_st;
layout (location = 5) out float out_mix;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	out_uv = a_uv;

	mat4 modelMatrix = u_transforms.models[gl_InstanceIndex];
	out_mix = modelMatrix[0][3];
	modelMatrix[0][3] = 0.0;

	out_normal = normalize((modelMatrix * vec4(a_normal, 0.0)).xyz);
	vec4 world_position = modelMatrix * vec4(a_position, 1.0);
	vec3 view_position = (u_shadow.view * world_position).xyz;

	out_view_depth = view_position.z;
	out_position = world_position.xyz;

	float t = u_ubo.u_time;
	out_st = 0.0025 * sin(t + (world_position.x + world_position.y) * 0.1) * world_position.z;

	//out_mix = float(gl_InstanceIndex & 1);

	gl_Position = u_push_const.camera_matrix * world_position;
}