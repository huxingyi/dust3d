#version 330
layout(location = 0) in vec4 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in float metalness;
layout(location = 5) in float roughness;
layout(location = 6) in vec3 tangent;
layout(location = 7) in float alpha;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform int normalMapEnabled;
out vec3 pointPosition;
out vec3 pointNormal;
out vec3 pointColor;
out vec2 pointTexCoord;
out float pointAlpha;
out float pointMetalness;
out float pointRoughness;
out mat3 pointTBN;

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