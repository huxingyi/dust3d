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

const vec3 LIGHT_DIR     = normalize(vec3(10.0, 15.0, 10.0));
const vec3 SNOW_COLOR    = vec3(1.0, 1.0, 1.0);
const float SNOW_BLEND   = 0.0;
const vec3 PAPER_TINT    = vec3(1.0, 1.0, 1.0);
const float MUTE_BLEND   = 0.0;
// Shadow: cool blue-grey
const vec3 SHADOW_TINT   = vec3(0.50, 0.52, 0.58);
// No specular
const float SPEC_THRESH  = 1.01;

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
    // Apply snow blend to base vertex colour first
    color = mix(color, SNOW_COLOR, SNOW_BLEND);
    // Then layer texture on top — text/labels render at full contrast
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

    // Clean cel-shade boundary
    float NdotL   = dot(normal, LIGHT_DIR);
    float litBand = smoothstep(0.0, 0.12, NdotL);

    // Lit: bright snow white; Shadow: cool blue
    vec3 litColor    = color;
    vec3 shadowColor = color * SHADOW_TINT;
    vec3 celColor    = mix(shadowColor, litColor, litBand);

    // Sparkle specular
    vec3 viewDir = normalize(eyePosition - pointPosition);
    vec3 halfVec = normalize(LIGHT_DIR + viewDir);
    float spec   = step(SPEC_THRESH, dot(normal, halfVec)) * litBand;
    celColor     = mix(celColor, vec3(1.0), spec * 0.90);

    // Cast shadow — very faint
    float shadow       = shadowCalculation(pointLightSpacePos);
    float shadowFactor = 1.0 - shadow * 0.20;

    vec3 finalColor = celColor * shadowFactor;
    finalColor = max(finalColor, color * 0.70);

    fragColor = vec4(finalColor, alpha);
}
