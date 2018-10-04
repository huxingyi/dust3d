#version 150
in vec4 vertex;
in vec3 normal;
in vec3 color;
in vec2 texCoord;
in float metalness;
in float roughness;
out vec3 vert;
out vec3 vertNormal;
out vec3 vertColor;
out vec2 vertTexCoord;
out float vertMetalness;
out float vertRoughness;
uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;
void main() 
{
    vert = vertex.xyz;
    vertNormal = normalMatrix * normal;
    vertColor = color;
    vertTexCoord = texCoord;
    vertMetalness = metalness;
    vertRoughness = roughness;
    gl_Position = projMatrix * mvMatrix * vertex;
}