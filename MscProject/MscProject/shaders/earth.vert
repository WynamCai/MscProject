#version 330

layout(location =0) in vec3 pos;
layout(location = 1)in vec3 normal;
layout(location = 2)in vec2 texCoord;

layout(location = 0)out vec3 outPos;
layout(location = 1)out vec2 outTex;
layout(location = 2)out vec3 outNormal;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;
uniform vec3 camPos;


void main()
{
	outPos = pos;
	outTex = texCoord;
	outNormal = normal;
	vec3 aPos = pos +camPos;
	gl_Position =proj * view * model*vec4(pos,1.0);
}