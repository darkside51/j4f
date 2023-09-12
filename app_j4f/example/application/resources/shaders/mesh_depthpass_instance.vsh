#version 450

#extension GL_ARB_shader_viewport_layer_array : enable

#define SHADOW_MAP_CASCADE_COUNT 3

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_tangent;
layout (location = 3) in vec2 a_uv;

layout (set = 0, binding = 0) readonly buffer static_SSBO {
	vec3 lightDirection;
	vec2 lightMinMax;
	vec4 lightColor;
	float saturation;
	mat4 models[100];
} u_constants;

layout (set = 2, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
	vec3 camera_position;
} u_shadow;

layout (set = 4, binding = 0) uniform UBO {
	float lighting;
	vec4 color;
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

// 	gl_Position = u_push_const.camera_matrix * u_constants.models[gl_InstanceIndex] * u_push_const.model_matrix * vec4(a_position, 1.0);

    int instanceIndex = gl_InstanceIndex % 100;
    gl_Layer = gl_InstanceIndex / 100;
    gl_Position = u_shadow.cascade_matrix[gl_Layer] * u_constants.models[instanceIndex] * u_push_const.model_matrix * vec4(a_position, 1.0);

	//for geometry shader if use it
	//gl_Position = u_constants.models[gl_InstanceIndex] * u_push_const.model_matrix * vec4(a_position, 1.0);
}