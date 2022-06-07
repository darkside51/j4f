#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outPos;

layout(push_constant) uniform matrix {
	mat4 mvp;
} pushConst;

void main()  {
	outPos = inPos; // model * inPos
	gl_Position = pushConst.mvp * vec4(inPos, 1.0);
	outUv = inUv;
}