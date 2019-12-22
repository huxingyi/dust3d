#version 330
layout(location = 0) in highp vec4 vertex;
layout(location = 1) in highp vec3 normal;
layout(location = 2) in highp vec3 color;
layout(location = 3) in highp vec2 texCoord;
layout(location = 4) in highp float metalness;
layout(location = 5) in highp float roughness;
layout(location = 6) in highp vec3 tangent;
layout(location = 7) in highp float alpha;
out highp vec3 vert;
out highp vec3 vertRaw;
out highp vec3 vertNormal;
out highp vec3 vertColor;
out highp vec2 vertTexCoord;
out highp float vertMetalness;
out highp float vertRoughness;
out highp vec3 cameraPos;
out highp vec3 firstLightPos;
out highp vec3 secondLightPos;
out highp vec3 thirdLightPos;
out highp float vertAlpha;
uniform highp mat4 projectionMatrix;
uniform highp mat4 modelMatrix;
uniform highp mat3 normalMatrix;
uniform highp mat4 viewMatrix;
uniform highp int normalMapEnabled;

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
    cameraPos = vec3(0, 0, -4.0);

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