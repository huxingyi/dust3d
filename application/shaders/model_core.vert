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
void main()
{
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vertex;
}