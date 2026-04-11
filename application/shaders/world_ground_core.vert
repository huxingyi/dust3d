#version 330
layout(location = 0) in vec4 vertex;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;
out vec3 pointPosition;
out vec4 pointLightSpacePos;

void main()
{
    vec4 worldVertex = modelMatrix * vertex;
    pointPosition = worldVertex.xyz;
    pointLightSpacePos = lightSpaceMatrix * worldVertex;
    gl_Position = projectionMatrix * viewMatrix * worldVertex;
}
