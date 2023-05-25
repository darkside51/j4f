#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

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
	float saturation;
} u_constants;

layout (set = 2, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
	vec3 camera_position;
} u_shadow;

layout(set = 4, binding = 0) uniform UBO {
	float lighting;
	vec4 color;
	mat4 skin_matrixes[192];
} u_ubo;

layout(push_constant) uniform PUSH_CONST {
	mat4 camera_matrix;
	mat4 model_matrix;
} u_push_const;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out float out_view_depth;
layout (location = 2) out vec3 out_position;
layout (location = 3) out vec3 out_halfwayDir;
layout (location = 4) out mat3 out_tbn;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	out_uv = a_uv;
	ivec4 joints = ivec4(a_joints);
	mat4 skin = u_ubo.skin_matrixes[joints.x] * a_weights.x
			 	+ u_ubo.skin_matrixes[joints.y] * a_weights.y
			  	+ u_ubo.skin_matrixes[joints.z] * a_weights.z
			  	+ u_ubo.skin_matrixes[joints.w] * a_weights.w;

	vec3 normal = normalize((u_push_const.model_matrix * (skin * vec4(a_normal, 0.0))).xyz);
	vec3 tangent = normalize((u_push_const.model_matrix * (skin * vec4(a_tangent.xyz, 0.0))).xyz);
	vec3 binormal = cross(normal, tangent) * a_tangent.w;
	out_tbn = mat3(tangent, binormal, normal);

	vec4 world_position = u_push_const.model_matrix * (skin * vec4(a_position, 1.0));
	vec3 view_position = (u_shadow.view * world_position).xyz;

	out_view_depth = view_position.z;
	out_position = world_position.xyz;
	gl_Position = u_push_const.camera_matrix * world_position;
	
	vec3 lightDir   = -u_constants.lightDirection;//normalize(-u_constants.lightDirection * 100000.0 - out_position);
	vec3 viewDir    = normalize(u_shadow.camera_position - out_position);
	out_halfwayDir 	= normalize(lightDir + viewDir);
}