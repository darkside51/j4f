#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_joints;
layout (location = 3) in vec4 a_weights;
layout (location = 4) in vec2 a_uv;

layout (set = 0, binding = 0) uniform UBO {
	int use_skin;
	mat4 skin_matrixes[192];
} u_ubo;

layout (set = 1, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
} u_shadow;

layout (set = 2, binding = 0) uniform static_lightUBO {
	vec3 lightDirection;
	vec2 lightMinMax;
	vec4 lightColor;
} u_constants;

layout(push_constant) uniform PUSH_CONST {
	mat4 camera_matrix;
	mat4 model_matrix;
} u_push_const;

layout (location = 0) out vec3 out_normal;
layout (location = 1) out vec2 out_uv;

layout (location = 2) out float out_view_depth;
layout (location = 3) out vec3 out_position;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	out_uv = a_uv;

	if (u_ubo.use_skin == 1) {
		mat4 skin = u_ubo.skin_matrixes[int(a_joints.x)] * a_weights.x
			 	  + u_ubo.skin_matrixes[int(a_joints.y)] * a_weights.y
			  	  + u_ubo.skin_matrixes[int(a_joints.z)] * a_weights.z
			  	  + u_ubo.skin_matrixes[int(a_joints.w)] * a_weights.w;

		out_normal = normalize((u_push_const.model_matrix * skin * vec4(a_normal, 0.0)).xyz);
		vec4 world_position = u_push_const.model_matrix * skin * vec4(a_position, 1.0);
		vec3 view_position = (u_shadow.view * world_position).xyz;

		out_view_depth = view_position.z;
		out_position = world_position.xyz;
		gl_Position = u_push_const.camera_matrix * world_position;

	} else {
		out_normal = normalize((u_push_const.model_matrix * vec4(a_normal, 0.0)).xyz);
		vec4 world_position = u_push_const.model_matrix * vec4(a_position, 1.0);
		vec3 view_position = (u_shadow.view * world_position).xyz;

		out_view_depth = view_position.z;
		out_position = world_position.xyz;
		gl_Position = u_push_const.camera_matrix * world_position;
	}
}