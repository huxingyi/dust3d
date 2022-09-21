#version 330
in vec3 pointPosition;
in vec3 pointNormal;
in vec3 pointColor;
in float pointAlpha;
in float pointMetalness;
in float pointRoughness;
out vec4 fragColor;
void main()
{
    fragColor = vec4(pointColor, pointAlpha);
}