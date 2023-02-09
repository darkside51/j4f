#version 450

layout (location = 0) in vec2 inUv;
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform UBO {
	float cascade;
} u_ubo;

layout (binding = 0, set = 1) uniform sampler2DArray u_texture;

void main() {	
	float d = texture(u_texture, vec3(inUv, u_ubo.cascade)).r;	
	outFragColor = vec4(d, d, d, 0.5);
}