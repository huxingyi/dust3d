#version 330
layout(location = 0) in vec4 vertex;
uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;
void main()
{
    gl_Position = lightSpaceMatrix * modelMatrix * vertex;
}
