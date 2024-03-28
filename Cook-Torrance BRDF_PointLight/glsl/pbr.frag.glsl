#version 330 core

uniform vec3 u_CamPos;

uniform vec3  u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AmbientOcclusion;
uniform int u_RustToggle;

// Varyings from vertex shader
in vec3 fs_Pos;
in vec3 fs_Nor;
out vec4 out_Col;

// Point lights
const vec3 light_pos[4] = vec3[](vec3(-10, 10, 10),
                                 vec3(10, 10, 10),
                                 vec3(-10, -10, 10),
                                 vec3(10, -10, 10));

const vec3 light_col[4] = vec3[](vec3(300.0, 300.0, 300.0),
                                 vec3(300.0, 300.0, 300.0),
                                 vec3(300.0, 300.0, 300.0),
                                 vec3(300.0, 300.0, 300.0));

const float PI = 3.14159265359;

vec3 reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 gammaCorrect(vec3 color) {
    return pow(color, vec3(1.0 / 2.2));
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    float D = a2 / denom;

    return max(D, 0.0);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    float ggx1 = geometrySchlickGGX(NdotV, roughness);

    return ggx1 * ggx2;
}


// cited: https://github.com/sol-prog/Perlin_Noise/blob/master/PerlinNoise.cpp
const int perm[256] = int[256](
  151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,
  20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,
  220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,
  200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,147,
  118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152, 2,44,
  154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,
  104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,
  114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
);

float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float noise(vec3 p) {
    int X = int(floor(p.x)) & 255;
    int Y = int(floor(p.y)) & 255;
    int Z = int(floor(p.z)) & 255;

    p.x -= floor(p.x);
    p.y -= floor(p.y);
    p.z -= floor(p.z);

    float u = fade(p.x);
    float v = fade(p.y);
    float w = fade(p.z);

    int A = (perm[X] + Y) & 255;
    int B = (perm[X + 1] + Y) & 255;

    return lerp(w, lerp(v, lerp(u, grad(perm[A + Z], p.x, p.y, p.z),
                                   grad(perm[B + Z], p.x - 1, p.y, p.z)),
                           lerp(u, grad(perm[A + Z + 1], p.x, p.y - 1, p.z),
                                   grad(perm[B + Z + 1], p.x - 1, p.y - 1, p.z))),
                   lerp(v, lerp(u, grad(perm[A + Z + 1], p.x, p.y, p.z - 1),
                                   grad(perm[B + Z + 1], p.x - 1, p.y, p.z - 1)),
                           lerp(u, grad(perm[A + Z + 2], p.x, p.y - 1, p.z - 1),
                                   grad(perm[B + Z + 2], p.x - 1, p.y - 1, p.z - 1))));
}

float fbm(vec3 p) {
    float total = 0.0;
    float amplitude = 1.0;
    for(int i = 0; i < 4; i++) { // Increase number of iterations for more detail
        total += noise(p) * amplitude;
        p *= 2.0;
        amplitude *= 0.5;
    }
    return total;
}

void main() {
    vec3 Lo = vec3(0.0);

    float noiseValue = fbm(fs_Pos * 5.0);
    float rustEffect = smoothstep(0.2, 0.8, noiseValue);

    float localMetallic = mix(u_Metallic, 0.1, rustEffect);
    float localRoughness = mix(u_Roughness, 0.8, rustEffect);

    for (int i = 0; i < 4; ++i) {
        // Calculate the irradiance from the light and the falloff
        vec3 l_pos = light_pos[i];
        vec3 l_col = light_col[i];
        vec3 irradiance = l_col / pow(length(l_pos - fs_Pos), 2.0);

        // Compute the view and light directions
        vec3 wi = normalize(l_pos - fs_Pos);
        vec3 wo = normalize(u_CamPos - fs_Pos);
        vec3 wh = normalize(wo + wi);

        // Fresnel term using Schlick's approximation
        vec3 R = mix(vec3(0.04), u_Albedo, u_Metallic);
        vec3 F = R + (1.0 - R) * pow(1.0 - max(dot(fs_Nor, wo), 0.0), 5.0);

        float D, G;
        // D and G term
        if (u_RustToggle != 0) {
            D = distributionGGX(fs_Nor, wh, localRoughness);
            G = geometrySmith(fs_Nor, wo, wi, localRoughness);
        }
        else{
            D = distributionGGX(fs_Nor, wh, u_Roughness);
            G = geometrySmith(fs_Nor, wo, wi, u_Roughness);
        }


        // Cook-Torrance specular component
        vec3 f_cook_torrance = (D * G * F) / (4.0 * dot(fs_Nor, wo) * dot(fs_Nor, wi));
        //f_cook_torrance = clamp(f_cook_torrance, 0.0, 1.0);

        // Schlick's approximation for ks
        vec3 ks = F;
        vec3 kd = (1.0 - ks) * (1.0 - u_Metallic);

        // Lambertian diffuse component
        vec3 f_lambert = u_Albedo * 0.31831015504887652430775499030746;

        // Combine to final
        vec3 f = kd * f_lambert + /*ks **/ f_cook_torrance;


        Lo += f * irradiance * max(dot(wi, fs_Nor), 0.0);
    }

    // Ambient light term
    vec3 ambient;
    if (u_RustToggle != 0) {
        float ao = u_AmbientOcclusion * (1.0 - rustEffect);
        ambient = 0.03 * ao * u_Albedo;
    } else {
        ambient = 0.03 * u_AmbientOcclusion * u_Albedo;
    }

    Lo = reinhard(Lo);   // Tone mapping
    Lo = gammaCorrect(Lo); // Gamma correction

    out_Col = vec4(Lo, 1.0);
}
