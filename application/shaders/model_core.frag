#version 330
uniform samplerCube environmentIrradianceMapId;
uniform samplerCube environmentSpecularMapId;
uniform sampler2D textureId;
uniform int textureEnabled;
uniform sampler2D normalMapId;
uniform int normalMapEnabled;
uniform sampler2D metalnessRoughnessAoMapId;
uniform int metalnessMapEnabled;
uniform int roughnessMapEnabled;
uniform int aoMapEnabled;
uniform vec3 eyePosition;
in vec3 pointPosition;
in vec3 pointNormal;
in vec3 pointColor;
in vec2 pointTexCoord;
in float pointAlpha;
in float pointMetalness;
in float pointRoughness;
in mat3 pointTBN;
out vec4 fragColor;

const float PI = 3.1415926;

vec3 fresnelSchlickRoughness(float NoV, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - NoV, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 color = pointColor;
    float alpha = pointAlpha;
    if (1 == textureEnabled) {
        vec4 textColor = texture(textureId, pointTexCoord);
        color = textColor.rgb;
        alpha = textColor.a;
    }
    vec3 normal = pointNormal;
    if (1 == normalMapEnabled) {
        normal = texture(normalMapId, pointTexCoord).rgb;
        normal = pointTBN * normalize(normal * 2.0 - 1.0);
    }
    float metalness = pointMetalness;
    if (1 == metalnessMapEnabled) {
        metalness = texture(metalnessRoughnessAoMapId, pointTexCoord).b;
    }
    float roughness = pointRoughness;
    if (1 == roughnessMapEnabled) {
        roughness = texture(metalnessRoughnessAoMapId, pointTexCoord).g;
    }
    float ambientOcclusion = 1.0;
    if (1 == aoMapEnabled) {
        ambientOcclusion = texture(metalnessRoughnessAoMapId, pointTexCoord).r;
    }

    vec3 n = normal;
    vec3 v = normalize(eyePosition - pointPosition);
    vec3 r = reflect(-v, n);

    float NoV = abs(dot(n, v)) + 1e-5;

    vec3 irradiance = texture(environmentIrradianceMapId, r).rgb;
    vec3 diffuse = irradiance * (1.0 - metalness) * pointColor;

    vec3 f0 = mix(vec3(0.04), pointColor, metalness);
    vec3 fresnelFactor = fresnelSchlickRoughness(NoV, f0, roughness);
    vec3 specular = fresnelFactor * texture(environmentSpecularMapId, r, 0.0).rgb;

    color = (diffuse + specular) * ambientOcclusion;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    fragColor = vec4(color, alpha);
}