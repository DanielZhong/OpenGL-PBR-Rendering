#version 330 core

uniform sampler2D u_AlbedoTexture;
uniform sampler2D u_MetallicTexture;
uniform sampler2D u_RoughnessTexture;

// Image-based lighting (only used if computing PBR reflection here)
uniform samplerCube u_DiffuseIrradianceMap;
uniform samplerCube u_GlossyIrradianceMap;
uniform sampler2D u_BRDFLookupTexture;

in vec3 fs_Pos;
in vec3 fs_Nor;
in vec2 fs_UV;

layout (location = 0) out vec4 gb_WorldSpacePosition;
layout (location = 1) out vec4 gb_Normal;
layout (location = 2) out vec3 gb_Albedo;
// R channel is metallic, G channel is roughness, B channel is mask
layout (location = 3) out vec3 gb_Metal_Rough_Mask;
layout (location = 4) out vec3 gb_PBR; // Optional

uniform vec3 u_CamPos;

void main()
{
    gb_WorldSpacePosition = vec4(fs_Pos, 1.0);
    gb_Normal = vec4(fs_Nor, 0.0);
    gb_Albedo = texture(u_AlbedoTexture, fs_UV).rgb;
    float metallic = texture(u_MetallicTexture, fs_UV).r;
    float roughness = texture(u_RoughnessTexture, fs_UV).r;
    gb_Metal_Rough_Mask = vec3(metallic, roughness, 1.0);

    vec3 N = normalize(fs_Nor); // Normalized normal vector
    vec3 wo = normalize(u_CamPos - fs_Pos); // View vector from camera to surface point
    vec3 albedo = texture(u_AlbedoTexture, fs_UV).rgb;


    // Fresnel term using Schlick's approximation
    vec3 F0 = mix(vec3(0.04), albedo, metallic); // Base reflectivity
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, wo), 0.0), 5.0);

    // Sample the BRDF lookup texture for the D and G terms
    vec2 brdfSample = texture(u_BRDFLookupTexture, vec2(max(dot(N, wo), 0.0), roughness)).rg;
    vec3 specular = vec3(0.0);

    // Calculate the reflection vector for glossy specular
    vec3 R = reflect(-wo, N); // Reflect wo about N to get wi
    const float maxMipLevels = 4.0;

    // Sample the glossy irradiance map using wi and determine the mip level based on roughness
    vec3 glossyIrradiance = textureLod(u_GlossyIrradianceMap, R, roughness * maxMipLevels).rgb;
    specular += glossyIrradiance * (F * brdfSample.x + brdfSample.y);

    // Compute kD as (1.0 - kS) and multiply with the diffuse component
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(u_DiffuseIrradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 color = kD * diffuse + specular;

    // Output to the G-buffer's PBR channel
    gb_PBR = color;
}
