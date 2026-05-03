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
    vec3 groundCenter = vec3(0.145, 0.145, 0.145);
    vec3 groundEdge   = vec3(0.10, 0.10, 0.10);

    float shadow = shadowCalculation(pointLightSpacePos);
    vec3 shadowTint = vec3(0.6, 0.6, 0.6);
    float stageDistance = length(pointPosition.xz);
    float stageFactor = 1.0 - smoothstep(12.0, 40.0, stageDistance);
    vec3 color = mix(groundEdge, groundCenter, stageFactor);
    color = mix(color, color * shadowTint, shadow * 0.38);

    fragColor = vec4(color, 1.0);
}
