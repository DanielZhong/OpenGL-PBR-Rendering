#version 330 core

uniform vec3 u_CamPos;

// PBR material attributes
uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AmbientOcclusion;
// Texture maps for controlling some of the attribs above, plus normal mapping
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_NormalMap;
// If true, use the textures listed above instead of the GUI slider values
uniform bool u_UseAlbedoMap;
uniform bool u_UseMetallicMap;
uniform bool u_UseRoughnessMap;
uniform bool u_UseAOMap;
uniform bool u_UseNormalMap;

// Image-based lighting
uniform samplerCube u_DiffuseIrradianceMap;
uniform samplerCube u_GlossyIrradianceMap;
uniform sampler2D u_BRDFLookupTexture;

// Varyings
in vec3 fs_Pos;
in vec3 fs_Nor;
in vec3 fs_Tan;
in vec3 fs_Bit;
in vec2 fs_UV;
out vec4 out_Col;

// Point lights
const vec3 light_pos[4] = vec3[](vec3(-10, 10, 10),
                                 vec3(10, 10, 10),
                                 vec3(-10, -10, 10),
                                 vec3(10, -10, 10));

const vec3 light_col[4] = vec3[](vec3(300.f, 300.f, 300.f),
                                 vec3(300.f, 300.f, 300.f),
                                 vec3(300.f, 300.f, 300.f),
                                 vec3(300.f, 300.f, 300.f));

const float PI = 3.14159f;

// Smith's Schlick-GGX approximation
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Trowbridge-Reitz GGX microfacet distribution
// An approximation of the Trowbridge-Reitz D() function from PBRT
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// F0 is surface reflection at zero incidence (looking head on)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    float ct = clamp(1.0 - cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * ((ct * ct) * (ct * ct) * ct);
}

// Same as above, but accounts for surface roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void coordinateSystem(in vec3 nor, out vec3 tan, out vec3 bit) {
    if (abs(nor.x) > abs(nor.y))
        tan = vec3(-nor.z, 0, nor.x) / sqrt(nor.x * nor.x + nor.z * nor.z);
    else
        tan = vec3(0, nor.z, -nor.y) / sqrt(nor.y * nor.y + nor.z * nor.z);
    bit = cross(nor, tan);
}

void main()
{
    vec3 N = fs_Nor;

    vec3 albedo = u_Albedo;
    if(u_UseAlbedoMap) {
        albedo = pow(texture(u_AlbedoMap, fs_UV).rgb, vec3(2.2));
    }
    float metallic = u_Metallic;
    if(u_UseMetallicMap) {
        metallic = texture(u_MetallicMap, fs_UV).r;
    }
    float roughness = u_Roughness;
    if(u_UseRoughnessMap) {
        roughness = texture(u_RoughnessMap, fs_UV).r;
    }
    float ambientOcclusion = u_AmbientOcclusion;
    if(u_UseAOMap) {
        ambientOcclusion = texture(u_AOMap, fs_UV).r;
    }
    if(u_UseNormalMap) {
        // Apply normal mapping
        vec3 mapNor = texture(u_NormalMap, fs_UV).rgb * 2.0 - vec3(1.0);
//        vec3 tan, bit;
//        coordinateSystem(N, tan, bit);
        N = mat3(fs_Tan, fs_Bit, N) * normalize(mapNor);
    }

    vec3 V = normalize(u_CamPos - fs_Pos);
    vec3 R = reflect(-V, N);

    vec3 Lo = vec3(0.f);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

#if 0
    for(int i = 4; i < 4; ++i) {
        vec3 L = normalize(light_pos[i] - fs_Pos);
        vec3 H = normalize(V + L);

        vec3 diff = light_pos[i] - fs_Pos;

        float dist2 = dot(diff, diff);
        float attenuation = 1.0 / dist2;

        vec3 radiance = light_col[i] * attenuation;

        // Assume white reflection of 0.04 for opaque dielectrics
        // As we become more metallic, albedo influences reflectivity more
        vec3 F0 = vec3(0.04);
        F0      = mix(F0, albedo, metallic);
        // Fresnel term
        vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

        // Normal distribution function
        float D = distributionGGX(N, H, roughness);
        // Geometric shadowing
        float G   = geometrySmith(N, V, L, roughness);

        // Torrance-Sparrow is DGF / (4 * cos(wo) * cos(wi))
        vec3 cook_torrance = D*G*F / (4.f * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.0001f);

        // Reflected light
        vec3 kS = F;
        // Refracted light
        vec3 kD = vec3(1.0) - kS;
        // Metal doesn't refract light, so we diminish kD by the metalness
        kD *= (1.0 - metallic);

        // absdot Lambert term of LTE
        float NdotL = max(dot(N, L), 0.0);

        // kD * albedo/PI is the diffuse reflection term ("Lambert")
        // cook_torrance is the specular reflection term
        // Combined they are the BSDF
        // radiance is Li
        // NdotL is absdot
        Lo += (kD * albedo / PI + cook_torrance) * radiance * NdotL;
    }
#endif
    // The above loop is effectively the integral over the hemisphere
    // for fs_Pos since we only care about direct light & there are
    // only point lights

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);

    // Sample our diffuse illumination & combine it with our albedo
    vec3 diffuseIrradiance = texture(u_DiffuseIrradianceMap, N).rgb;
    vec3 diffuse = diffuseIrradiance * albedo;

    // Sample the glossy irradiance map & the BRDF lookup texture
    // Combine these values via the split-sum approximation
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 glossyIrradiance = textureLod(u_GlossyIrradianceMap, R,
                                       roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(u_BRDFLookupTexture, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = glossyIrradiance * (F * brdf.x + brdf.y);

//    out_Col = vec4(specular, 1.);
//    out_Col = out_Col / (out_Col + vec4(1.0));
//    // Gamma correction
//    out_Col = pow(out_Col, vec4(1.0/2.2));
//    out_Col.a = 1.;
//    return;

    vec3 ambient = (kD * diffuse + specular) * ambientOcclusion;
    vec3 color = ambient + Lo;

    // Reinhard operator to reduce HDR values from magnitude of 100s back to [0, 1]
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    out_Col = vec4(color, 1.f);
}
