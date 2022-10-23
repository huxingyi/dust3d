#version 110
uniform sampler2D environmentIrradianceMapId[6];
uniform sampler2D environmentSpecularMapId[6];
uniform sampler2D textureId;
uniform int textureEnabled;
uniform sampler2D normalMapId;
uniform int normalMapEnabled;
uniform sampler2D metalnessRoughnessAoMapId;
uniform int metalnessMapEnabled;
uniform int roughnessMapEnabled;
uniform int aoMapEnabled;
uniform vec3 eyePosition;
varying vec3 pointPosition;
varying vec3 pointNormal;
varying vec3 pointColor;
varying vec2 pointTexCoord;
varying float pointAlpha;
varying float pointMetalness;
varying float pointRoughness;
varying mat3 pointTBN;

const float PI = 3.1415926;

vec3 fresnelSchlickRoughness(float NoV, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - NoV, 0.0, 1.0), 5.0);
}

void cubemap(vec3 r, out float texId, out vec2 st)
{
   vec3 uvw;
   vec3 absr = abs(r);
   if (absr.x > absr.y && absr.x > absr.z) {
     // x major
     float negx = step(r.x, 0.0);
     uvw = vec3(r.zy, absr.x) * vec3(mix(-1.0, 1.0, negx), -1, 1);
     texId = negx;
   } else if (absr.y > absr.z) {
     // y major
     float negy = step(r.y, 0.0);
     uvw = vec3(r.xz, absr.y) * vec3(1.0, mix(1.0, -1.0, negy), 1.0);
     texId = 2.0 + negy;
   } else {
     // z major
     float negz = step(r.z, 0.0);
     uvw = vec3(r.xy, absr.z) * vec3(mix(1.0, -1.0, negz), -1, 1);
     texId = 4.0 + negz;
   }
   st = vec2(uvw.xy / uvw.z + 1.) * .5;
}

vec4 texturesAsCube(in sampler2D maps[6], in vec3 direction)
{
    float texId;
    vec2 st;
    cubemap(direction, texId, st);
    vec4 color = vec4(0);
    {
        vec4 side = texture2D(maps[0], st);
        float select = step(0.0 - 0.5, texId) * step(texId, 0.0 + 0.5);
        color = mix(color, side, select);
    }
    {
        vec4 side = texture2D(maps[1], st);
        float select = step(1.0 - 0.5, texId) * step(texId, 1.0 + 0.5);
        color = mix(color, side, select);
    }
    {
        vec4 side = texture2D(maps[2], st);
        float select = step(2.0 - 0.5, texId) * step(texId, 2.0 + 0.5);
        color = mix(color, side, select);
    }
    {
        vec4 side = texture2D(maps[3], st);
        float select = step(3.0 - 0.5, texId) * step(texId, 3.0 + 0.5);
        color = mix(color, side, select);
    }
    {
        vec4 side = texture2D(maps[4], st);
        float select = step(4.0 - 0.5, texId) * step(texId, 4.0 + 0.5);
        color = mix(color, side, select);
    }
    {
        vec4 side = texture2D(maps[5], st);
        float select = step(5.0 - 0.5, texId) * step(texId, 5.0 + 0.5);
        color = mix(color, side, select);
    }
    return color;
}

void main()
{
    vec3 color = pointColor;
    float alpha = pointAlpha;
    if (1 == textureEnabled) {
        vec4 textColor = texture2D(textureId, pointTexCoord);
        color = textColor.rgb;
        alpha = textColor.a;
    }
    vec3 normal = pointNormal;
    if (1 == normalMapEnabled) {
        normal = texture2D(normalMapId, pointTexCoord).rgb;
        normal = pointTBN * normalize(normal * 2.0 - 1.0);
    }
    float metalness = pointMetalness;
    if (1 == metalnessMapEnabled) {
        metalness = texture2D(metalnessRoughnessAoMapId, pointTexCoord).b;
    }
    float roughness = pointRoughness;
    if (1 == roughnessMapEnabled) {
        roughness = texture2D(metalnessRoughnessAoMapId, pointTexCoord).g;
    }
    float ambientOcclusion = 1.0;
    if (1 == aoMapEnabled) {
        ambientOcclusion = texture2D(metalnessRoughnessAoMapId, pointTexCoord).r;
    }

    vec3 n = normal;
    vec3 v = normalize(eyePosition - pointPosition);
    vec3 r = reflect(-v, n);

    float NoV = abs(dot(n, v)) + 1e-5;

    vec3 irradiance = texturesAsCube(environmentIrradianceMapId, r).rgb;
    vec3 diffuse = irradiance * (1.0 - metalness) * color;

    vec3 f0 = mix(vec3(0.04), color, metalness);
    vec3 fresnelFactor = fresnelSchlickRoughness(NoV, f0, roughness);
    vec3 specular = fresnelFactor * texturesAsCube(environmentSpecularMapId, r).rgb;

    color = (diffuse + specular) * ambientOcclusion;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    gl_FragColor = vec4(color, alpha);
}
