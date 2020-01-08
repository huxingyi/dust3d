#version 430

uniform mat4 uModelViewMatrix;
// uniform mat4 uNormalMatrix;
uniform mat4 uProjectionMatrix;

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

out vec3 vNormal;
out vec2 vTexCoord;

void main(){
    vNormal = aNormal;
    vTexCoord = aTexCoord;
    vec4 position = vec4(aPosition, 1.0);
    gl_Position = uProjectionMatrix * uModelViewMatrix * position;
}