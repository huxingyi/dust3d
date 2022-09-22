#version 330
layout(location = 0) in vec4 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in float metalness;
layout(location = 5) in float roughness;
layout(location = 6) in vec3 tangent;
layout(location = 7) in float alpha;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 eyePosition;
out vec3 pointPosition;
out vec3 pointNormal;
out vec3 pointColor;
out float pointAlpha;
out float pointMetalness;
out float pointRoughness;

void main()
{
    pointPosition = (modelMatrix * vertex).xyz;
    pointNormal = normalize((modelMatrix * vec4(normal, 1.0)).xyz);
    pointColor = color;
    pointAlpha = alpha;
    pointMetalness = metalness;
    pointRoughness = roughness;

    gl_Position = projectionMatrix * viewMatrix * vec4(pointPosition, 1.0);
}