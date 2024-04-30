#version 330 core

uniform ivec2 u_Dimensions;
uniform float u_Time;
uniform sampler2D u_Texture;
uniform sampler2D u_Albedo;
uniform sampler2D u_NormalTexture;

in vec2 fs_UV;

out vec4 fragColor;

mat2 makeRotationMatrix(in float angle) {
    float cosTheta = cos(angle);
    float sinTheta = sin(angle);
    return mat2(cosTheta, -sinTheta, sinTheta, cosTheta);
}

vec2 generateRandomVector(vec2 seed) {
    return fract(sin(vec2(dot(seed, vec2(127.1, 311.7)), dot(seed, vec2(269.5, 183.3)))) * 43758.5453);
}

float generateNoise(vec2 uv) {
    vec2 uvInteger = floor(uv);
    vec2 uvFraction = fract(uv);

    float minimumDistance = 1.0;
    float rotationTime = cos(float(u_Time * 0.001));

    for(int y = -1; y <= 1; y++) {
        for(int x = -1; x <= 1; x++) {
            vec2 neighborOffset = vec2(float(x), float(y));
            vec2 randomPoint = generateRandomVector(uvInteger + neighborOffset);
            vec2 difference = neighborOffset + randomPoint - uvFraction;
            float distance = length(difference);

            if(distance < minimumDistance) {
                minimumDistance = distance;
                randomPoint.x = cos(rotationTime) * randomPoint.x + sin(rotationTime) * randomPoint.y;
                randomPoint.y = -sin(rotationTime) * randomPoint.x + cos(rotationTime) * randomPoint.y;
            }
        }
    }

    return cos(minimumDistance * 3.14159265 * 0.7);
}

vec2 computeGradient(vec2 position) {
    float epsilon = .05;
    float gradX = generateNoise(vec2(position.x + epsilon, position.y)) - generateNoise(vec2(position.x - epsilon, position.y));
    float gradY = generateNoise(vec2(position.x, position.y + epsilon)) - generateNoise(vec2(position.x, position.y - epsilon));
    return vec2(gradX, gradY);
}

float calculateFlow(vec2 position) {
    float intensityScale = 2.0;
    float reducedZ = 0.0;
    vec2 basePosition = position;

    for (float i = 1.0; i < 7.0; i++) {
        position += u_Time * 0.001 * 0.6;
        basePosition += u_Time * 0.001 * 1.9;
        vec2 gradient = computeGradient(i * position * 0.34 + u_Time * 0.001 * 1.0);
        gradient *= makeRotationMatrix(u_Time * 0.001 * 6.0 - (0.05 * position.x + 0.03 * position.y) * 40.0);
        position += gradient * 0.5;
        reducedZ += (sin(generateNoise(position) * 7.0) * 0.5 + 0.5) / intensityScale;
        position = mix(basePosition, position, 0.77);
        intensityScale *= 1.4;
        position *= 2.0;
        basePosition *= 1.9;
    }
    return reducedZ;
}

void main()
{
        vec2 pos = fs_UV - 0.5;
        pos.x *= u_Dimensions.x / u_Dimensions.y;
        pos *= 2.5f;
        float reducedZ = calculateFlow(pos);

        vec3 col = vec3(.5, 0.08, 0.02) / reducedZ;
        fragColor = vec4(texture(u_Albedo, fs_UV).rgb + col, 1);
}
