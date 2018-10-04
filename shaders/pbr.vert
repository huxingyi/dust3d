attribute vec4 vertex;
attribute vec3 normal;
attribute vec3 color;
attribute vec2 texCoord;
attribute float metalness;
attribute float roughness;
varying vec3 vert;
varying vec3 vertNormal;
varying vec3 vertColor;
varying vec2 vertTexCoord;
varying float vertMetalness;
varying float vertRoughness;
varying vec3 vertView;
uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;
void main() 
{
    vert = vertex.xyz;
    vertNormal = normalize((projMatrix * mvMatrix * vec4(normal, 1.0)).xyz);
    vertColor = color;
    vertTexCoord = texCoord;
    vertMetalness = metalness;
    vertRoughness = roughness;
    vertView = (projMatrix * mvMatrix * vec4(0, 0, 0, 1.0)).xyz;
    gl_Position = projMatrix * mvMatrix * vertex;
}