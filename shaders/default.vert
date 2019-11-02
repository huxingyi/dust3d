attribute highp vec4 vertex;
attribute highp vec3 normal;
attribute highp vec3 color;
attribute highp vec2 texCoord;
attribute highp float metalness;
attribute highp float roughness;
attribute highp vec3 tangent;
attribute highp float alpha;
varying highp vec3 vert;
varying highp vec3 vertRaw;
varying highp vec3 vertNormal;
varying highp vec3 vertColor;
varying highp vec2 vertTexCoord;
varying highp float vertMetalness;
varying highp float vertRoughness;
varying highp vec3 cameraPos;
varying highp vec3 firstLightPos;
varying highp vec3 secondLightPos;
varying highp vec3 thirdLightPos;
varying highp float vertAlpha;
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