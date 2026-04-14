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
    float scale = 2.0;
    float checkerX = floor((pointPosition.x + groundOffset.x) * scale);
    float checkerZ = floor((pointPosition.z + groundOffset.y) * scale);
    float checker = mod(checkerX + checkerZ, 2.0);
    vec3 groundColor = mix(vec3(0.92, 0.92, 0.92), vec3(0.98, 0.98, 0.98), checker);

    float shadow = shadowCalculation(pointLightSpacePos);
    vec3 shadowTint = vec3(0.70, 0.70, 0.75);
    vec3 color = mix(groundColor, groundColor * shadowTint, shadow * 0.65);

    fragColor = vec4(color, 1.0);
}
