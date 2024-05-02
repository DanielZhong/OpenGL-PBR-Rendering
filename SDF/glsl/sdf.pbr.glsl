
// TODO add any helper functions you need here

vec3 metallic_plastic_LTE(BSDF bsdf, vec3 wo) {
    vec3 N = bsdf.nor;
    vec3 albedo = bsdf.albedo;
    float metallic = bsdf.metallic;
    float roughness = bsdf.roughness;
    float ambientOcclusion = bsdf.ao;

    // TODO
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

    vec3 color = (kD * diffuse + specular) * ambientOcclusion;
    return color;
}
