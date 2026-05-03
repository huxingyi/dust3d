#version 110
// Inverted-hull outline — GLSL 110
attribute vec4 vertex;
attribute vec3 normal;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
// Width of the outline in clip-space units
uniform float outlineWidth;

void main()
{
    vec4 clipPos = projectionMatrix * viewMatrix * modelMatrix * vertex;

    // Transform normal to view space to get a screen-space offset direction
    mat3 modelView3 = mat3(viewMatrix * modelMatrix);
    vec3 viewNormal = normalize(modelView3 * normal);

    // Offset in NDC by pushing along the projected normal
    vec2 screenOffset = normalize(viewNormal.xy + vec2(0.0001)) * outlineWidth * clipPos.w;
    clipPos.xy += screenOffset;

    gl_Position = clipPos;
}
