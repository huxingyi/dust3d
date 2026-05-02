#version 330
uniform sampler2D shadowMap;
uniform vec2 groundOffset;
in vec3 pointPosition;
in vec4 pointLightSpacePos;
out vec4 fragColor;

float shadowCalculation(vec4 lightSpacePos)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 0.0;
    float bias = 0.001;
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
    vec3 groundColor = vec3(0.98, 0.80, 0.64);
    vec3 backdropColor = vec3(0.22, 0.18, 0.16);

    float shadow = shadowCalculation(pointLightSpacePos);
    vec3 shadowTint = vec3(0.82, 0.82, 0.84);
    float stageDistance = length(pointPosition.xz);
    float stageFactor = 1.0 - smoothstep(12.0, 40.0, stageDistance);
    stageFactor = stageFactor * stageFactor * stageFactor;
    vec3 color = mix(backdropColor, groundColor, stageFactor);
    color = mix(color, color * shadowTint, shadow * 0.5);
    color = max(color, vec3(0.20));

    fragColor = vec4(color, 1.0);
}
