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
varying vec3 cameraPos;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
void main()
{
    vert = (modelMatrix * vertex).xyz;
    vertNormal = normalize((modelMatrix * vec4(normal, 1.0)).xyz);
    vertColor = color;
    vertTexCoord = texCoord;
    vertMetalness = metalness;
    vertRoughness = roughness;
    cameraPos = vec3(0, 0, -2.1);
    gl_Position = projectionMatrix * viewMatrix * vec4(vert, 1.0);
}