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
	vec4 mp = vec4(vp_matrix[gl_InvocationID][0][2], vp_matrix[gl_InvocationID][1][2], vp_matrix[gl_InvocationID][2][2], vp_matrix[gl_InvocationID][3][2]);
	float strokeWidth = 0.325;
	
	for (int i = 0; i < gl_in.length(); i++) {
		out_uv = in_uv[i];
		out_view_depth = in_view_depth[i];
		out_position = in_position[i];
		out_halfwayDir = in_halfwayDir[i];
		out_tbn = in_tbn[i];
		out_stroke = float(gl_InvocationID);

		vec3 normal = out_tbn[2];
		out_position.z += strokeWidth;
		vec3 position = out_position + out_stroke * (normal * strokeWidth);
		vec4 p = vp_matrix[gl_InvocationID] * vec4(position, 1.0);
		
		float simpleZ = dot(mp, vec4(out_position, 1.0));
		p.z = max(p.z + 0.02 * out_stroke, simpleZ);

		gl_Position = p;
		EmitVertex();
	}
	EndPrimitive();
}