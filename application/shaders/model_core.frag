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

// Defined as a constant for a fixed "Top-Right" light source
// X: 10.0 (Right), Y: 15.0 (High Up), Z: 10.0 (Slightly in front of objects)
const vec3 LIGHT_POS = vec3(10.0, 15.0, 10.0);

// Soft cool gray for shadows
const vec3 SHADOW_TINT = vec3(0.82, 0.81, 0.85);

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

    vec3 lightDir = normalize(LIGHT_POS - pointPosition);

    // Soft "Half-Lambert" Diffuse
    // This maps dot product from [-1, 1] to [0.5, 1.0] 
    // This ensures the "dark side" is never pitch black
    float diff = dot(normal, lightDir) * 0.25 + 0.75;

    // --- Hemispherical Component ---
    // Objects are slightly darker on their underside to simulate contact with the floor
    float hemi = smoothstep(-0.2, 1.0, normal.y);
    vec3 ambient = mix(SHADOW_TINT, vec3(1.0), hemi);

    // Combine light contribution
    vec3 lighting = ambient * diff;

    color = color * lighting;

    // Apply a light gamma correction for the "washed out" architectural feel
    fragColor = vec4(pow(color, vec3(1.0 / 1.1)), alpha);
}