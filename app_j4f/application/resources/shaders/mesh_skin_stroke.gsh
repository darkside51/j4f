#version 450

layout (triangles, invocations = 2) in;
layout (triangle_strip, max_vertices = 3) out;

in gl_PerVertex {
	vec4 gl_Position;
} gl_in[];

out gs_out {
	layout (location = 0) out vec2 out_uv;
	layout (location = 1) out float out_view_depth;
	layout (location = 2) out vec3 out_position;
	layout (location = 3) out vec3 out_halfwayDir;
	layout (location = 4) out mat3 out_tbn;
	layout (location = 7) out float out_stroke;
};

layout (location = 0) in vec2 in_uv[];
layout (location = 1) in float in_view_depth[];
layout (location = 2) in vec3 in_position[];
layout (location = 3) in vec3 in_halfwayDir[];
layout (location = 4) in mat3 in_tbn[];
layout (location = 7) in mat4 vp_matrix[];

void main() {
	for (int i = 0; i < gl_in.length(); i++) {
		out_uv = in_uv[i];
		out_view_depth = in_view_depth[i];
		out_position = in_position[i];
		out_halfwayDir = in_halfwayDir[i];
		out_tbn = in_tbn[i];

		vec3 normal = out_tbn[2];
		vec3 position = out_position + gl_InvocationID * (normal * 0.35f);

		gl_Position = vp_matrix[gl_InvocationID] * vec4(position, 1.0);
		out_stroke = float(gl_InvocationID);

		float depth = gl_Position.z / gl_Position.w;

		//gl_Position.z += (1.0 - depth) * 2.25 * out_stroke;
		gl_Position.z += 0.05 * out_stroke;
		EmitVertex();
	}
	EndPrimitive();
}