#version 450

#extension GL_ARB_shader_viewport_layer_array : enable

#define SHADOW_MAP_CASCADE_COUNT 3

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_joints;
layout (location = 3) in vec4 a_weights;
layout (location = 4) in vec2 a_uv;

layout (set = 0, binding = 0) uniform static_shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
	vec3 camera_position;
} u_shadow;

layout (set = 1, binding = 0) uniform UBO {
	mat4 skin_matrixes[192];
} u_ubo;

layout(push_constant) uniform PUSH_CONST {
	mat4 camera_matrix;
	mat4 model_matrix;
} u_push_const;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	ivec4 joints = ivec4(a_joints);
	mat4 skin = u_ubo.skin_matrixes[joints.x] * a_weights.x
			 	+ u_ubo.skin_matrixes[joints.y] * a_weights.y
			  	+ u_ubo.skin_matrixes[joints.z] * a_weights.z
			  	+ u_ubo.skin_matrixes[joints.w] * a_weights.w;

	gl_Position = u_shadow.cascade_matrix[gl_InstanceIndex] * u_push_const.model_matrix * skin * vec4(a_position, 1.0);
    gl_Layer = gl_InstanceIndex;
}