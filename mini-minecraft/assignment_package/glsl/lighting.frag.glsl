#version 330 core

in vec2 fs_UV;

uniform vec3 u_lightDir;
uniform sampler2D u_Albedo;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_FogTexture;
uniform sampler2D u_ShadowTexture;
uniform sampler2D u_SkyTexture;
uniform sampler2D u_WorldPosTexture;
uniform vec3 u_CameraPos;

layout (location = 0) out vec4 gb_Lighting;

const vec3 fogColor = vec3(0.5, 0.6, 0.7);
const float specularIntensity = 1.5; // Specular intensity for Blinn-Phong model
const float shininess = 20.0;

void main() {
    vec4 diffuseColor = texture(u_Albedo, fs_UV);
    vec3 skyColor = texture(u_SkyTexture, fs_UV).rgb;
    float skyAlpha = texture(u_SkyTexture, fs_UV).a;

    // Early return for fully transparent albedo
    if (diffuseColor.rgb == vec3(0.0, 0.0, 0.0)) {
        gb_Lighting = vec4(skyColor, skyAlpha);
        return;
    }

    // Fetching world position from the texture
    vec3 worldPos = texture(u_WorldPosTexture, fs_UV).rgb;
    vec3 normal = normalize(texture(u_NormalTexture, fs_UV).rgb);
    vec3 lightDir = normalize(-u_lightDir);
    vec3 viewDir = normalize(u_CameraPos - worldPos); // Calculate view direction
    vec3 halfVector = normalize(lightDir + viewDir); // Compute halfway vector

    float diffuseTerm = max(dot(normal, lightDir), 0.0);
    float specularTerm = pow(max(dot(normal, halfVector), 0.0), shininess) * specularIntensity;
    float ambientTerm = 0.25;

    // Combine diffuse, specular, and ambient terms
    float lightIntensity_water = texture(u_ShadowTexture, fs_UV).r * (diffuseTerm + specularTerm) + ambientTerm;
    float lightIntensity = texture(u_ShadowTexture, fs_UV).r * (diffuseTerm + 0.3) + ambientTerm;

    // Fog blending using linear interpolation
    float fogFactor = texture(u_FogTexture, fs_UV).r;
    vec3 colorWithFog = mix(diffuseColor.rgb * lightIntensity, skyColor, fogFactor);
    vec3 colorWithFog_water = mix(diffuseColor.rgb * lightIntensity_water, skyColor, fogFactor);
    float finalAlpha = mix(diffuseColor.a, skyAlpha, fogFactor); // Blend alpha values similarly

    if(texture(u_FogTexture, fs_UV).g == 1){ // water
        gb_Lighting = vec4(colorWithFog_water, finalAlpha);
    } else{
        gb_Lighting = vec4(colorWithFog, finalAlpha);
    }
}
