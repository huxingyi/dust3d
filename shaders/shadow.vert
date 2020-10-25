#version 110
/*
 * This file follow the Stackoverflow content license: CC BY-SA 4.0,
 * since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
*/
attribute vec3 qt_Vertex;
uniform mat4 qt_LightViewProjectionMatrix;
const float c_one = 1.0;

void main(void)
{
    gl_Position = qt_LightViewProjectionMatrix * vec4(qt_Vertex, c_one);
}

