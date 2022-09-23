#version 330
in vec3 pointColor;
in float pointAlpha;
out vec4 fragColor;
void main()
{
    fragColor = vec4(pointColor, pointAlpha);
}