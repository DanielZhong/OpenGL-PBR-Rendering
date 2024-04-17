#version 330 core

// [0] is the specular reflection.
// [4] is the diffuse reflection.
// [1][2][3] are intermediate levels of glossy reflection.
uniform sampler2D u_TexSSR[5];

uniform sampler2D u_TexPositionWorld;
uniform sampler2D u_TexNormal;
uniform sampler2D u_TexAlbedo;
uniform sampler2D u_TexMetalRoughMask;

uniform samplerCube u_DiffuseIrradianceMap;
uniform samplerCube u_GlossyIrradianceMap;
uniform sampler2D u_BRDFLookupTexture;

uniform vec3 u_CamPos;

in vec2 fs_UV;

out vec4 out_Col;

const float PI = 3.14159f;

vec3 computeLTE(vec3 pos, vec3 N,
                vec3 albedo, float metallic, float roughness,
                vec3 wo,
                vec4 Li_Diffuse,
                vec4 Li_Glossy) {
    // TODO: Implement this based on your PBR shader code.
    // Don't apply the Reinhard operator or gamma correction;
    // they should be applied at the end of main().

    // When you evaluate the Diffuse BSDF portion of your LTE,
    // the Li term should be a LERP between Li_Diffuse and the
    // color in u_DiffuseIrradianceMap based on the alpha value
    // of Li_Diffuse.

    // Likewise, your Microfacet BSDF portion's Li will be a mix
    // of Li_Glossy and u_GlossyIrradianceMap's color based on
    // Li_Glossy.a

    // Everything else will be the same as in the code you
    // wrote for the previous assignment.

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, wo), 0.0), 5.0);

    // Diffuse light interaction
    vec3 diffuseIrradiance = mix(texture(u_DiffuseIrradianceMap, N).rgb, Li_Diffuse.rgb, Li_Diffuse.a);
    // vec3 diffuse = texture(u_DiffuseIrradianceMap, N).rgb * albedo * (1.0 - metallic);
    vec3 diffuse = diffuseIrradiance * albedo * (1.0 - metallic);

    // Specular light interaction
    vec3 R = reflect(-wo, N);
    float mipLevel = roughness * 4.0;
    vec3 glossyIrradiance = mix(textureLod(u_GlossyIrradianceMap, R, mipLevel).rgb, Li_Glossy.rgb, Li_Glossy.a);

    vec2 brdfSample = texture(u_BRDFLookupTexture, vec2(max(dot(N, wo), 0.0), roughness)).rg;
    vec3 specular = glossyIrradiance * (F * brdfSample.x + brdfSample.y);

    // Combine diffuse and specular components
    return (diffuse + specular);
}

void main() {
    // TODO: Combine all G-buffer textures into your final
    // output color. Compared to the environment-mapped
    // PBR shader, you will have two additional Li terms.

    // One represents your diffuse screen reflections, sampled
    // from the last index in the u_TexSSR sampler2D array.

    // The other represents your glossy screen reflections,
    // interpolated between two levels of glossy reflection stored
    // in the lower indices of u_TexSSR. Your interpolation t will
    // be dependent on your roughness.
    // For example, if your roughness were 0.1, then your glossy
    // screen-space reflected color would be:
    // mix(u_TexSSR[0], u_TexSSR[1], fract(0.1 * 4))
    // If roughness were 0.9, then your color would be:
    // mix(u_TexSSR[2], u_TexSSR[3], fract(0.9 * 4))


    out_Col = vec4(texture2D(u_TexSSR[0], fs_UV).rgb, 1.0);

    vec3 albedo = pow(texture2D(u_TexAlbedo, fs_UV).rgb, vec3(2.2));
    float metallic = texture(u_TexMetalRoughMask, fs_UV).r;
    float roughness = texture(u_TexMetalRoughMask, fs_UV).g;
    float visibility = texture(u_TexMetalRoughMask, fs_UV).b;
    vec3 N = normalize(texture(u_TexNormal, fs_UV).rgb);
    vec3 wo = normalize(u_CamPos - texture(u_TexPositionWorld, fs_UV).xyz);

    // Fetch SSR data
    vec4 Li_Diffuse = texture(u_TexSSR[4], fs_UV); // Diffuse reflection
    int idx = int(4.0 * roughness);
    float t = fract(4.0 * roughness);
    vec4 Li_Glossy = mix(texture(u_TexSSR[idx], fs_UV), texture(u_TexSSR[idx + 1], fs_UV), t); // Glossy reflection

    // Compute LTE
    vec3 color = computeLTE(texture(u_TexPositionWorld, fs_UV).xyz, N, albedo, metallic, roughness, wo, Li_Diffuse, Li_Glossy);

    // Apply tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    out_Col = vec4(color, visibility);
    // texture(u_TexSSR[0], fs_UV);
    // out_Col = vec4(Li_Glossy.a, 0, 0, 1);
}
