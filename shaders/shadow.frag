#version 110
/*
 * This file follow the Stackoverflow content license: CC BY-SA 4.0,
 * since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
*/
void main(void)
{
    gl_FragDepth = gl_FragCoord.z;
}

