#version 330
layout(location = 0) in vec4 vertex;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 lightSpaceMatrix;
out vec3 pointPosition;
out vec4 pointLightSpacePos;

void main()
{
    pointPosition = vertex.xyz;
    pointLightSpacePos = lightSpaceMatrix * vertex;
    gl_Position = projectionMatrix * viewMatrix * vertex;
}
