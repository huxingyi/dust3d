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
const vec3 SHADOW_TINT = vec3(0.82, 0.81, 0.85);

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
    // A negative texture coordinate marks a vertex without UV (the bones in the animation
    // preview): it keeps its own color and is not affected by the maps.
    bool hasUv = pointTexCoord.x > -0.5;
    if (1 == textureEnabled && hasUv) {
        vec4 textColor = texture(textureId, pointTexCoord);
        color = textColor.rgb;
        alpha = textColor.a;
    }
    vec3 normal = pointNormal;
    if (1 == normalMapEnabled && hasUv) {
        normal = texture(normalMapId, pointTexCoord).rgb;
        normal = pointTBN * normalize(normal * 2.0 - 1.0);
    }

    vec3 lightDir = normalize(LIGHT_POS - pointPosition);

    // Soft Half-Lambert diffuse
    float diff = dot(normal, lightDir) * 0.25 + 0.75;

    // Hemispherical ambient
    float hemi = smoothstep(-0.2, 1.0, normal.y);
    vec3 ambient = mix(SHADOW_TINT, vec3(1.0), hemi);

    float shadow = shadowCalculation(pointLightSpacePos);
    vec3 lighting = ambient * diff * (1.0 - shadow * 0.4);

    vec3 baseColor = color;
    color = color * lighting;

    // Part materials (metallic, roughness, glow) from the material map: a preview of what the
    // exported model shows in a PBR engine, kept in this viewer's soft style.
    if ((1 == metalnessMapEnabled || 1 == roughnessMapEnabled || 1 == aoMapEnabled) && hasUv) {
        vec4 material = texture(metalnessRoughnessAoMapId, pointTexCoord);
        float metal = (1 == metalnessMapEnabled) ? material.b : 0.0;
        float rough = (1 == roughnessMapEnabled) ? material.g : 1.0;
        float sky = smoothstep(-0.4, 1.0, normal.y);
        vec3 metalColor = baseColor * (0.30 + 0.85 * sky);
        color = mix(color, metalColor, metal);
        float spec = pow(max(dot(normal, lightDir), 0.0), mix(48.0, 6.0, rough)) * (1.0 - rough) * 0.7;
        color += mix(vec3(1.0), baseColor, metal) * spec;
        color = mix(color, baseColor * 1.25 + vec3(0.08), 1.0 - material.a);
    }

    fragColor = vec4(pow(color, vec3(1.0 / 1.1)), alpha);
}
