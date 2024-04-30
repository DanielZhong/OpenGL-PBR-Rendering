#version 330 core
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform vec4 u_Color; // The color with which to render this instance of geometry.
uniform bool u_Transparent;
uniform sampler2DShadow  u_DepthTexture;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_Col;
in vec2 fs_UV;
in vec2 fs_Animated;
in vec3 fs_Tan; // Surface tangent
in vec3 fs_Bit; // Surface bitangent
in vec4 shadow_coord;
in vec4 fs_WorldPos;

uniform sampler2D u_Texture;
uniform float u_Time;
uniform sampler2D u_NormalTexture;
uniform vec3 u_CameraPos;

layout (location = 0) out vec4 gb_Albedo;
layout (location = 1) out vec4 gb_Normal;
layout (location = 2) out vec4 gb_WorldPos;
layout (location = 3) out vec4 gb_Fog;
layout (location = 4) out vec4 gb_Shadow;

const float fogNear = 50.0;                 // The distance at which the fog starts
const float fogFar = 100.0;

float fresnelFunction(vec3 viewDir, vec3 normal, float minTransparency, float maxTransparency) {
    float cosTheta = clamp(dot(normalize(viewDir), normalize(normal)), 0.0, 1.0);
    float fresnelEffect = mix(maxTransparency, minTransparency, pow(1.0 - cosTheta, 3.0));
    return fresnelEffect;
}

void main() {
    gb_WorldPos = fs_WorldPos;
    vec3 normalTex = texture(u_NormalTexture, fs_UV).rgb * 2.0 - 1.0;
    mat3 TBN = mat3(normalize(fs_Tan), normalize(fs_Bit), normalize(fs_Nor));
    vec3 normal = normalize(TBN * normalTex);
    if(texture(u_NormalTexture, fs_UV).rgb == vec3(1, 1, 1)){
        gb_Normal = vec4(fs_Nor.rgb, 1);
    } else{
        gb_Normal = vec4(normal, 1);
    }

    vec3 viewDirection = normalize(u_CameraPos - fs_WorldPos.xyz);
    float TransAlpha = fresnelFunction(viewDirection, fs_Nor.rgb, 1., 0.2);
    // Shadow
    vec3 shadow_proj = shadow_coord.xyz / shadow_coord.w;
    shadow_proj = shadow_proj * 0.5 + 0.5;
    vec2 poissonDisk[16] = vec2[](
            vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
            vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
            vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
            vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
            vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
            vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
            vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
            vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
            );
    int samples = 16;  // Number of unique Poisson Disk samples
    int multiSampleFactor = 5; // Number of times each disk sample is used
    float bias = 0.0045;  // Shadow mapping bias
    float shadow = 1.0;
    float reductionFactor = 1.5 / (float(samples) * float(multiSampleFactor));

    for (int i = 0; i < samples; ++i) {
        for (int j = 0; j < multiSampleFactor; ++j) {
            float scale = 1.0 + 0.1 * float(j); // Scales each sample slightly differently
            vec2 sampleCoord = shadow_proj.xy + scale * poissonDisk[i] / 700.0;
            float pcfDepth = texture(u_DepthTexture, vec3(sampleCoord, shadow_proj.z));
            if (pcfDepth < shadow_proj.z - bias) {
                shadow -= reductionFactor;  // Reduce shadow contribution for each depth fail
            }
        }
    }
    shadow = clamp(shadow, 0.25, 1.0);


    // Modulate albedo color with shadow visibility
    vec4 albedoColor = texture(u_Texture, fs_UV);

    float alphaValue = u_Transparent ? TransAlpha : albedoColor.a;
    if (u_Transparent && albedoColor.rgb == vec3(0, 0, 0)){
        discard;
    }

    // Calculate fog factor based on depth
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float fogFactor = clamp((depth - fogNear) / (fogFar - fogNear), 0.0, 1.0);

    if(u_Transparent){
        gb_Fog = vec4(fogFactor, 1, fogFactor, 1);
    } else {
        gb_Fog = vec4(fogFactor, 0, fogFactor, 1);
    }

    gb_Shadow = vec4(shadow, shadow, shadow, 1);

    // Output the final color with alpha
    gb_Albedo = vec4(albedoColor.rgb, alphaValue);
}
