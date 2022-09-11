#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout (triangles, invocations = SHADOW_MAP_CASCADE_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;

in gl_PerVertex {
	vec4 gl_Position;
} gl_in[];

out gs_out {
	layout (location = 0) vec2 out_uv;
};

layout (location = 0) in vec2 in_uv[];

layout (set = 2, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
} u_shadow;

void main() {
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;
		out_uv = in_uv[i];
		gl_Position = u_shadow.cascade_matrix[gl_InvocationID] * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}