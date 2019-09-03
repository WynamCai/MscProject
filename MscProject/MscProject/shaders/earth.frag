#version 330

layout(location = 0)in vec3 inPos;
layout(location = 1)in vec2 texCoord;
layout(location = 2)in vec3 normal;

uniform sampler2D texture1;
out vec4 outColor;

void main()
{
vec2 longitudeLatitude = vec2((atan(texCoord.y, texCoord.x) / 3.1415926 + 1.0) * 0.5,
                                  (asin(0) / 3.1415926 + 0.5));
	outColor = texture(texture1,longitudeLatitude);
}