#version 110
uniform vec3 eyePosition;
uniform sampler2D environmentIrradianceMapId[6];
uniform sampler2D environmentSpecularMapId[6];
varying vec3 pointPosition;
varying vec3 pointNormal;
varying vec3 pointColor;
varying float pointAlpha;
varying float pointMetalness;
varying float pointRoughness;

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
    for (int i = 0; i < 6; ++i) {
        vec4 side = texture2D(maps[i], st);
        float select = step(float(i) - 0.5, texId) * step(texId, float(i) + 0.5);
        color = mix(color, side, select);
    }
    return color;
}

void main()
{
    vec3 n = pointNormal;
    vec3 v = normalize(eyePosition - pointPosition);
    vec3 r = reflect(-v, n);

    float NoV = abs(dot(n, v)) + 1e-5;

    vec3 irradiance = texturesAsCube(environmentIrradianceMapId, r).rgb;
    vec3 diffuse = irradiance * (1.0 - pointMetalness) * pointColor;

    vec3 f0 = mix(vec3(0.04), pointColor, pointMetalness);
    vec3 fresnelFactor = fresnelSchlickRoughness(NoV, f0, pointRoughness);
    vec3 specular = fresnelFactor * texturesAsCube(environmentSpecularMapId, r).rgb;

    vec3 color = diffuse + specular;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    gl_FragColor = vec4(color, pointAlpha);
}
