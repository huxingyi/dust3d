#version 330
uniform vec3 eyePosition;
uniform samplerCube environmentIrradianceMapId;
uniform samplerCube environmentSpecularMapId;
in vec3 pointPosition;
in vec3 pointNormal;
in vec3 pointColor;
in float pointAlpha;
in float pointMetalness;
in float pointRoughness;
out vec4 fragColor;

const float PI = 3.1415926;

vec3 fresnelSchlickRoughness(float NoV, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - NoV, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 n = pointNormal;
    vec3 v = normalize(eyePosition - pointPosition);
    vec3 r = reflect(-v, n);

    float NoV = abs(dot(n, v)) + 1e-5;

    vec3 irradiance = texture(environmentIrradianceMapId, r).rgb;
    vec3 diffuse = irradiance * (1.0 - pointMetalness) * pointColor;

    vec3 f0 = mix(vec3(0.04), pointColor, pointMetalness);
    vec3 fresnelFactor = fresnelSchlickRoughness(NoV, f0, pointRoughness);
    vec3 specular = fresnelFactor * texture(environmentSpecularMapId, r, 0.0).rgb;

    vec3 color = diffuse + specular;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    fragColor = vec4(color, pointAlpha);
}