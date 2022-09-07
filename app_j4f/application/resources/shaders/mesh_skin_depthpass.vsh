#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_tangent;
layout (location = 3) in vec4 a_joints;
layout (location = 4) in vec4 a_weights;
layout (location = 5) in vec2 a_uv;

layout (set = 0, binding = 0) uniform static_lightUBO {
	vec3 lightDirection;
	vec2 lightMinMax;
	vec4 lightColor;
} u_constants;

layout (set = 2, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
} u_shadow;

layout (set = 4, binding = 0) uniform UBO {
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

	if (u_ubo.use_skin == 1) {
		mat4 skin = u_ubo.skin_matrixes[int(a_joints.x)] * a_weights.x
			 	  + u_ubo.skin_matrixes[int(a_joints.y)] * a_weights.y
			  	  + u_ubo.skin_matrixes[int(a_joints.z)] * a_weights.z
			  	  + u_ubo.skin_matrixes[int(a_joints.w)] * a_weights.w;

		//gl_Position = u_push_const.camera_matrix * u_push_const.model_matrix * skin * vec4(a_position, 1.0);
		//for geometry shader if use it
		gl_Position = u_push_const.model_matrix * skin * vec4(a_position, 1.0);
	} else {
		//gl_Position = u_push_const.camera_matrix * u_push_const.model_matrix * vec4(a_position, 1.0);
		//for geometry shader if use it
		gl_Position = u_push_const.model_matrix * vec4(a_position, 1.0);
	}
}