#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 0) out vec3 outColor;

layout(push_constant) uniform matrix {
	mat4 mvp;
} pushConst;

void main()  {
	gl_Position = pushConst.mvp * vec4(inPos, 1.0);
	outColor = inColor;
}