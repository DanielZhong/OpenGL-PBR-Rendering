#version 330 core

uniform vec4 u_Color;
uniform sampler2D u_Texture;
uniform float u_Time;

in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec2 fs_UV;
in vec2 fs_Animated;

uniform sampler2D u_Albedo;

uniform sampler2D u_NormalTexture;

out vec4 fragColor;

float generateRandomNoise(vec3 position) {
    return fract(sin(dot(position, vec3(127.1, 311.7, 191.999)))
                 * 43758.5453);
}

float interpolateSmoothly(float start, float end, float weight) {
    weight = smoothstep(0, 1, weight);
    return mix(start, end, weight);
}

float cubicInterpolation(vec3 position) {
    vec3 fractionalPart = fract(position);
    float lowerLeftBack = generateRandomNoise(floor(position) + vec3(0,0,0));
    float lowerRightBack = generateRandomNoise(floor(position) + vec3(1,0,0));
    float upperLeftBack = generateRandomNoise(floor(position) + vec3(0,1,0));
    float upperRightBack = generateRandomNoise(floor(position) + vec3(1,1,0));

    float lowerLeftFront = generateRandomNoise(floor(position) + vec3(0,0,1));
    float lowerRightFront = generateRandomNoise(floor(position) + vec3(1,0,1));
    float upperLeftFront = generateRandomNoise(floor(position) + vec3(0,1,1));
    float upperRightFront = generateRandomNoise(floor(position) + vec3(1,1,1));

    float backLowerMix = interpolateSmoothly(lowerLeftBack, lowerRightBack, fractionalPart.x);
    float backUpperMix = interpolateSmoothly(upperLeftBack, upperRightBack, fractionalPart.x);
    float frontLowerMix = interpolateSmoothly(lowerLeftFront, lowerRightFront, fractionalPart.x);
    float frontUpperMix = interpolateSmoothly(upperLeftFront, upperRightFront, fractionalPart.x);

    float lowerMix = interpolateSmoothly(backLowerMix, frontLowerMix, fractionalPart.z);
    float upperMix = interpolateSmoothly(backUpperMix, frontUpperMix, fractionalPart.z);

    return interpolateSmoothly(lowerMix, upperMix, fractionalPart.y);
}

float fbm(vec3 position) {
    float amplitude = 0.5;
    float frequency = 4.0;
    float total = 0.0;
    for(int i = 0; i < 8; i++) {
        total += cubicInterpolation(position * frequency) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return total;
}

void main()
{
    vec4 texture_color = texture(u_Albedo, fs_UV);
    texture_color = texture_color * (0.5 * fbm(fs_Pos.xyz) + 0.5);

    vec4 water_color = vec4(0.2, 0.6, 0.9, 1.0);

    vec4 t = vec4(u_Time) * 0.03;
    t.xy = fs_UV * 6.0;
    float val1 = length(0.5 - fract(t.xyw *= mat3(vec3(-2.0, -1.0, 0.0), vec3(3.0, -1.0, 1.0), vec3(1.0, -1.0, -1.0)) * 0.5));
    float val2 = length(0.5 - fract(t.xyw *= mat3(vec3(-2.0, -1.0, 0.0), vec3(3.0, -1.0, 1.0), vec3(1.0, -1.0, -1.0)) * 0.3));
    float val3 = length(0.5 - fract(t.xyw *= mat3(vec3(-2.0, -1.0, 0.0), vec3(3.0, -1.0, 1.0), vec3(1.0, -1.0, -1.0)) * 0.5));

    vec4 color = (pow(min(min(val1, val2), val3), 5.0) * 3.0) + texture_color * water_color * 1.5;
    fragColor = vec4 (color.rgb, texture_color.a);
}
