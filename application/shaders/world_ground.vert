#version 110
attribute vec4 vertex;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;
varying vec3 pointPosition;
varying vec4 pointLightSpacePos;

void main()
{
    vec4 worldVertex = modelMatrix * vertex;
    pointPosition = worldVertex.xyz;
    pointLightSpacePos = lightSpaceMatrix * worldVertex;
    gl_Position = projectionMatrix * viewMatrix * worldVertex;
}
