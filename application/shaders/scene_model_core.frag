#version 330
uniform sampler2D shadowMap;
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
in vec4 pointLightSpacePos;
out vec4 fragColor;

const vec3 LIGHT_POS = vec3(10.0, 15.0, 10.0);
const vec3 SHADOW_TINT = vec3(0.98, 0.96, 0.94);

float shadowCalculation(vec4 lightSpacePos)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 0.0;
    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / 2048.0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main()
{
    vec3 color = pointColor;
    float alpha = pointAlpha;
    if (1 == textureEnabled) {
        vec4 textColor = texture(textureId, pointTexCoord);
        color = mix(color, textColor.rgb, textColor.a);
        alpha = max(alpha, textColor.a);
    }
    vec3 normal = pointNormal;
    if (1 == normalMapEnabled) {
        normal = texture(normalMapId, pointTexCoord).rgb;
        normal = pointTBN * normalize(normal * 2.0 - 1.0);
    }

    vec3 lightDir = normalize(vec3(-10.0, -15.0, -10.0));
    float nDotL = max(dot(normal, -lightDir), 0.0);
    float lightCoverage = smoothstep(0.10, 0.60, nDotL);
    float lightFalloff = mix(0.70, 1.35, nDotL);
    float fillLight = 0.65 + 0.35 * lightCoverage;

    float hemi = smoothstep(-0.1, 1.0, normal.y);
    vec3 ambient = mix(SHADOW_TINT, vec3(1.0), hemi);

    float shadow = shadowCalculation(pointLightSpacePos);
    float range = length(pointPosition - eyePosition);
    float rangeFactor = 1.0 - smoothstep(12.0, 26.0, range);
    rangeFactor = max(rangeFactor, 0.80);

    vec3 lighting = ambient * lightFalloff * fillLight * rangeFactor * (1.0 - shadow * 0.18);
    lighting = max(lighting, vec3(0.35));

    fragColor = vec4(pow(color * lighting, vec3(1.0 / 1.1)), alpha);
}
