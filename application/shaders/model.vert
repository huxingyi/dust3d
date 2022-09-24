#version 110
attribute vec4 vertex;
attribute vec3 normal;
attribute vec3 color;
attribute vec2 texCoord;
attribute float metalness;
attribute float roughness;
attribute vec3 tangent;
attribute float alpha;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform int normalMapEnabled;
varying vec3 pointPosition;
varying vec3 pointNormal;
varying vec3 pointColor;
varying vec2 pointTexCoord;
varying float pointAlpha;
varying float pointMetalness;
varying float pointRoughness;
varying mat3 pointTBN;

mat3 transpose(mat3 m) 
{
    return mat3(m[0][0], m[1][0], m[2][0],
                m[0][1], m[1][1], m[2][1],
                m[0][2], m[1][2], m[2][2]);
}

void main()
{
    pointPosition = (modelMatrix * vertex).xyz;
    pointNormal = normalize((modelMatrix * vec4(normal, 1.0)).xyz);
    pointColor = color;
    pointTexCoord = texCoord;
    pointAlpha = alpha;
    pointMetalness = metalness;
    pointRoughness = roughness;

    gl_Position = projectionMatrix * viewMatrix * vec4(pointPosition, 1.0);

    if (1 == normalMapEnabled) {
        vec3 T = normalize(normalMatrix * tangent);
        vec3 N = normalize(normalMatrix * normal);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);
        pointTBN = mat3(T, B, N);
    }
}
