#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (triangles, invocations = SHADOW_MAP_CASCADE_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;

layout (set = 2, binding = 0) uniform shadowUBO {
	vec4 cascade_splits;
	mat4 cascade_matrix[SHADOW_MAP_CASCADE_COUNT];
	mat4 view;
} u_shadow;

layout (location = 1) in int inInstanceIndex[];

void main() {
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;
		gl_Position = u_shadow.cascade_matrix[gl_InvocationID] * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}