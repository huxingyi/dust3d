#version 110
/*
 * This file follow the Stackoverflow content license: CC BY-SA 4.0,
 * since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
*/
struct directional_light
{
    vec3 direction;
    vec3 eye;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct material_properties
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float specularPower;
    float opacity;
    float brightness;
};

uniform directional_light qt_Light;
uniform material_properties qt_Material;
uniform sampler2D qt_ShadowMap;
uniform bool qt_ShadowEnabled;

varying vec4 v_Normal;
varying vec4 v_ShadowPosition;

const float c_zNear = 0.1;
const float c_ZFar = 1000.0;
const float c_zero = 0.0;
const float c_one = 1.0;
const float c_half = 0.5;
const float textureSize = 2048.0;
const vec2 texelSize = 1.0 / vec2(textureSize,textureSize);

vec4 evaluateLightMaterialColor(in vec4 normal)
{
    // Start with black color
    vec3 finalColor = vec3(c_zero, c_zero, c_zero);

    // Upgrade black color to the base ambient color
    finalColor += qt_Light.ambient.rgb * qt_Material.ambient.rgb;

    // Add diffuse component to it
    vec4 lightDir = vec4( normalize(qt_Light.direction), 0.0 );
    float diffuseFactor = max( c_zero, dot(lightDir, normal) );
    if(diffuseFactor > c_zero)
    {
        finalColor += qt_Light.diffuse.rgb *
                      qt_Material.diffuse.rgb *
                      diffuseFactor *
                      qt_Material.brightness;
    }

    // Add specular component to it
    const vec3 blackColor = vec3(c_zero, c_zero, c_zero);
    if( !(qt_Material.specular.rgb == blackColor || qt_Light.specular.rgb == blackColor || qt_Material.specularPower == c_zero) )
    {
        vec4 viewDir = vec4( normalize(qt_Light.eye), 0.0 );
        vec4 reflectionVec = reflect(lightDir, normal);
        float specularFactor = max( c_zero, dot(reflectionVec, -viewDir) );
        if(specularFactor > c_zero)
        {
            specularFactor = pow( specularFactor, qt_Material.specularPower );
            finalColor += qt_Light.specular.rgb *
                          qt_Material.specular.rgb *
                          specularFactor;
        }
    }

    // All done!
    return vec4( finalColor, qt_Material.opacity );
}


float linearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * c_zNear * c_ZFar) / (c_ZFar + c_zNear - z * (c_ZFar - c_zNear));
}

float evaluateShadow(in vec4 shadowPos)
{
    vec3 shadowCoords = shadowPos.xyz / shadowPos.w;
    shadowCoords = shadowCoords * c_half + c_half;
    if(shadowCoords.z > c_one)
        return c_one;

    float currentDepth = shadowPos.z;

    float shadow = c_zero;
    const int sampleRange = 2;
    const float nrSamples = (2.0*float(sampleRange) + 1.0)*(2.0*float(sampleRange) + 1.0);
    for(int x=-sampleRange; x<=sampleRange; x++)
    {
        for(int y=-sampleRange; y<=sampleRange; y++)
        {
            vec2 pcfCoords = shadowCoords.xy + vec2(x,y)*texelSize;
            float pcfDepth = linearizeDepth( texture2D(qt_ShadowMap, pcfCoords).r );
            shadow += (currentDepth < pcfDepth) ? c_one : c_half;
        }
    }

    shadow /= nrSamples;

    return shadow;
}

void main(void)
{
    vec4 lmColor = evaluateLightMaterialColor(v_Normal);
    if(qt_ShadowEnabled == true)
    {
        float shadow = evaluateShadow(v_ShadowPosition);
        gl_FragColor = vec4(lmColor.xyz * shadow, qt_Material.opacity);
    }
    else
        gl_FragColor = lmColor;
}

