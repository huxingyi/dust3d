#version 110
/*
 * This file follow the Stackoverflow content license: CC BY-SA 4.0,
 * since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
*/
attribute vec4 qt_Vertex;
attribute vec4 qt_Normal;

uniform mat4 qt_NormalMatrix;
uniform mat4 qt_LightViewProjectionMatrix;
uniform mat4 qt_ModelViewProjectionMatrix;

varying vec4 v_Normal;
varying vec4 v_ShadowPosition;

void main(void)
{
    v_Normal = normalize(qt_NormalMatrix * qt_Normal);
    v_ShadowPosition = qt_LightViewProjectionMatrix * vec4(qt_Vertex.xyz, 1.0);

    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;
}

