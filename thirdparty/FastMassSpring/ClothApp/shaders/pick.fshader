#version 430

in vec2 vTexCoord;
out vec4 fragColor;

uniform int uTessFact;

void main(){
    float vx = round(vTexCoord.x * (uTessFact - 1));
    float vy = round(vTexCoord.y * (uTessFact - 1));
    fragColor = vec4(vx / (uTessFact - 1), vy / (uTessFact - 1), 0.2, 1.0);
}