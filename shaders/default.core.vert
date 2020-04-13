#version 330
layout(location = 0) in vec4 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in float metalness;
layout(location = 5) in float roughness;
layout(location = 6) in vec3 tangent;
layout(location = 7) in float alpha;
out vec3 vert;
out vec3 vertRaw;
out vec3 vertNormal;
out vec3 vertColor;
out vec2 vertTexCoord;
out float vertMetalness;
out float vertRoughness;
out vec3 cameraPos;
out vec3 firstLightPos;
out vec3 secondLightPos;
out vec3 thirdLightPos;
out float vertAlpha;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform int normalMapEnabled;
uniform vec3 eyePos;

mat3 transpose(mat3 m) 
{
    return mat3(m[0][0], m[1][0], m[2][0],
                m[0][1], m[1][1], m[2][1],
                m[0][2], m[1][2], m[2][2]);
}

void main()
{
    vert = (modelMatrix * vertex).xyz;
    vertRaw = vert;
    vertNormal = normalize((modelMatrix * vec4(normal, 1.0)).xyz);
    vertColor = color;
    vertAlpha = alpha;
    cameraPos = eyePos;

    firstLightPos = vec3(5.0, 5.0, 5.0);
    secondLightPos = vec3(-5.0, 5.0, 5.0);
    thirdLightPos = vec3(0.0, -5.0, -5.0);

    gl_Position = projectionMatrix * viewMatrix * vec4(vert, 1.0);

    if (normalMapEnabled == 1) {
        vec3 T = normalize(normalMatrix * tangent);
        vec3 N = normalize(normalMatrix * normal);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);
        
        mat3 TBN = transpose(mat3(T, B, N));
        firstLightPos = TBN * firstLightPos;
        secondLightPos = TBN * secondLightPos;
        thirdLightPos = TBN * thirdLightPos;
        cameraPos = TBN * cameraPos;
        vert  = TBN * vert;
    }

    vertTexCoord = texCoord;
    vertMetalness = metalness;
    vertRoughness = roughness;
}