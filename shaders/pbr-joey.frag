// Copy and modified from Joey's code
// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.1.lighting/1.1.pbr.fs

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

varying highp vec3 vert;
varying highp vec3 vertNormal;
varying highp vec3 vertColor;
varying highp vec2 vertTexCoord;
varying highp float vertMetalness;
varying highp float vertRoughness;
varying highp vec3 cameraPos;
uniform highp vec3 lightPos;
uniform highp sampler2D textureId;
uniform highp int textureEnabled;

const int MAX_LIGHTS = 8;
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};
int lightCount;
Light lights[MAX_LIGHTS];

void main() 
{
    const highp float vertAmbientOcclusion = 1.0;

    vec3 albedo = vertColor;
    if (textureEnabled == 1) {
        albedo = texture2D(textureId, vertTexCoord).rgb;
    }
    albedo = pow(albedo, vec3(2.2));

    lightCount = 3;
    float roughness = vertRoughness;
    float metalness = vertMetalness;

    // Key light 
    lights[0].position = vec3(5.0, 5.0, 5.0);
    lights[0].color = vec3(150.0, 150.0, 150.0);
    lights[0].intensity = 0.8;

    // Fill light
    lights[1].position = vec3(-5.0, 5.0, 5.0);
    lights[1].color = vec3(150.0, 150.0, 150.0);
    lights[1].intensity = 0.4;

    // Rim light
    lights[2].position = vec3(0.0, -2.5, -5.0);
    lights[2].color = vec3(150.0, 150.0, 150.0);
    lights[2].intensity = 0.2;

    vec3 N = normalize(vertNormal);
    vec3 V = normalize(cameraPos - vert);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < lightCount; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lights[i].position - vert);
        vec3 H = normalize(V + L);
        float distance = length(lights[i].position - vert);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights[i].color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 nominator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalness;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += lights[i].intensity * (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * vertAmbientOcclusion;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    gl_FragColor = vec4(color, 1.0);
}