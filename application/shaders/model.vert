#version 110
attribute vec4 vertex;
attribute vec3 normal;
attribute vec3 color;
attribute vec2 texCoord;
attribute float metalness;
attribute float roughness;
attribute vec3 tangent;
attribute float alpha;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 eyePosition;
varying vec3 pointPosition;
varying vec3 pointNormal;
varying vec3 pointColor;
varying float pointAlpha;
varying float pointMetalness;
varying float pointRoughness;

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
