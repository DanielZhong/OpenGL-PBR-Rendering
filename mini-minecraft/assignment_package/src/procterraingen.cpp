#include "procterraingen.h"

#include <math.h>
#include <iostream>
#include <glm_includes.h>

const float PI = 3.141593;

ProcTerrainGen::ProcTerrainGen() {}

int ProcTerrainGen::getHeight(int x, int z) {
    float grass = grasslands(x, z);
    float mtn = mountains(x, z);

    float perlin = glm::smoothstep(0.7, 0.9, (double)(fbm(x/1024.f, z/1024.f, 0.2))+1)/2;

    return glm::mix(grass, mtn, perlin);
}

float ProcTerrainGen::mountains(float x, float z) {

    int mountainMin = 155;
    int mountainMax = 300;

    float h = 0;
    float amp = 0.5f;
    float scale = 1024;
    for (int i = 0; i < 4; ++i) {
        float h1 = perlinNoise(glm::vec2(x / scale, z / scale));
        h1 = (h1 + 1) / 2;
        h += h1 * amp;
        amp *= 0.5;
        scale *= 0.5;
    }

    h = glm::smoothstep(0.7, 0.9, (double)h);
    h = sin(h);

    return mountainMin + (mountainMax - mountainMin) * h ;
}

float ProcTerrainGen::grasslands(float x, float z) {
    x /= 256;
    z /= 256;

    int grassMin = 128;
    int grassMax = 154;
    float h = (fbm(x, z, 0.5) + 1) / 2;


    return grassMin + (grassMax - grassMin) * h ;
}

float ProcTerrainGen::caves(float x, float y, float z){
    float perlin = perlinNoise3D(glm::vec3(x / 30 , y / 30, z / 30));
    return perlin;
}

float ProcTerrainGen::snowlands(float x, float z) {
    x /= 128;
    z /= 128;

    int snowlandMin = 150;
    int snowlandMax = 170;
    float perlin = (perlinNoise(glm::vec2(x + 32, z + 54)) + 1) / 2;
    return snowlandMin + (snowlandMax - snowlandMin) * perlin;
}

float ProcTerrainGen::desert(float x, float z) {
    x /= 32;
    z /= 32;

    int desertMin = 135;
    int desertMax = 145;
    float perlin = (perlinNoise(glm::vec2(x + 15, z + 107)) + 1) / 2;
    return desertMin + (desertMax - desertMin) * perlin;
}

BiomeType ProcTerrainGen::getTerrainType(float t, float m) {
    if (t > 0.9) {
        if (m > 0.9) {
            return DESERT;
        } else {
            return GRASSLAND;
        }
    } else {
        if (m > 0.9) {
            return MOUNTAIN;
        } else {
            return SNOWLAND;
        }
    }
}

float ProcTerrainGen::biomeInterp(glm::vec4 heights, glm::vec2 mt)
{
    // interpolate using temperature
    // snowland & grassland
    float h1 = glm::mix(heights.x, heights.y, mt.y);
    // mountain & desert
    float h2 = glm::mix(heights.z, heights.w, mt.y);

    // interpolate using moisture
    return glm::mix(h1, h2, mt.x);
}

float ProcTerrainGen::worleyNoise(float x, float z) {
    float intX, fractX;
    fractX = modf(x, &intX);

    float intZ, fractZ;
    fractZ = modf(z, &intZ);

    float minDist1 = 1;
    float minDist2 = 1;

    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            glm::vec2 neighborDirection = glm::vec2(j, i);
            glm::vec2 neighborCtr = rand2(glm::vec2(intX, intZ) + neighborDirection);
            glm::vec2 diff = neighborDirection + neighborCtr - glm::vec2(fractX, fractZ);

            float dist = glm::length(diff);

            if (dist < minDist1) {
                minDist2 = minDist1;
                minDist1 = dist;
            } else if (dist < minDist2) {
                minDist2 = dist;
            }
        }
    }

    return minDist2 - minDist1;
}

glm::vec2 ProcTerrainGen::rand2(glm::vec2 c) {
    float x = glm::fract(glm::sin(glm::dot(c, glm::vec2(127.1, 311.7))) * 43758.5453);
    float z = glm::fract(glm::sin(glm::dot(c, glm::vec2(420.2, 1337.1))) * 789221.1234);

    return glm::vec2(x, z);
}

glm::vec3 ProcTerrainGen::rand3(glm::vec3 c) {
    float x = 4096.0 * sin(glm::dot(c, glm::vec3(17.0, 59.4, 15.0)));
    glm::vec3 randomNum;
    randomNum.z = glm::fract(512.0 * x);
    x *= .125;
    randomNum.x = glm::fract(512.0 * x);
    x *= .125;
    randomNum.y = glm::fract(512.0 * x);
    return randomNum - glm::vec3(0.5);
}

float ProcTerrainGen::fbm(float x, float z, float persistence) {
    float total = 0;
    int octaves = 8;

    for (int i = 0; i < octaves; i++) {
        float frequency = pow(2, i);
        float amplitude = pow(persistence, i);

        total += perlinNoise(glm::vec2(x * frequency, z * frequency)) * amplitude;
    }

    return total;
}

float ProcTerrainGen::fbm3D(float x, float y, float z, float persistence) {
    float total = 0;
    int octaves = 8;

    for (int i = 0; i < octaves; i++) {
        float frequency = pow(2, i);
        float amplitude = pow(persistence, i);

        total += perlinNoise3D(glm::vec3(x * frequency, y * frequency, z * frequency)) * amplitude;
    }

    return total;
}

float ProcTerrainGen::perlinNoise(glm::vec2 uv) {
    float surfletSum = 0.f;

    for (int dx = 0; dx <= 1; ++dx) {
        for (int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, glm::floor(uv) + glm::vec2(dx, dy));
        }
    }

    return surfletSum;
}

float ProcTerrainGen::perlinNoise3D(glm::vec3 p){
    float surfletSum = 0.f;

    for (int dx = 0; dx <= 1; ++dx) {
        for (int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <=1; ++dz){
                surfletSum += surflet3D(p, glm::floor(p) + glm::vec3(dx, dy, dz));
            }
        }
    }

    return surfletSum + 0.1;
}

glm::vec2 pow(glm::vec2 v, int power) {
    for (int i = 0; i < power; i++) {
        v *= v;
    }

    return v;
}

glm::vec3 pow(glm::vec3 v, int power) {
    glm::vec3 p = v;
    for (int i = 0; i < power-1; i++) {
        p *= v;
    }
    return p;
}

float ProcTerrainGen::surflet(glm::vec2 p, glm::vec2 gridPoint) {
    glm::vec2 t2 = glm::abs(p - gridPoint);
    glm::vec2 t = glm::vec2(1.f) - 6.f * pow(t2, 5) + 15.f * pow(t2, 4) - 10.f * pow(t2, 3);

    glm::vec2 gradient = noiseNormalVector(gridPoint) * 2.f - glm::vec2(1,1);
    glm::vec2 diff = p - gridPoint;

    float height = glm::dot(diff, gradient);

    return height * t.x * t.y;
}

float ProcTerrainGen::surflet3D(glm::vec3 p, glm::vec3 gridPoint) {

    glm::vec3 t2    = glm::abs(p - gridPoint);
    glm::vec3 t     = glm::vec3(1.f) - 6.f * pow(t2, 5.f) + 15.f * pow(t2, 4.f) - 10.f * pow(t2, 3.f);

    glm::vec3 gradient = rand3(gridPoint) * 2.f - glm::vec3(1.f);
    glm::vec3 diff = p - gridPoint;

    float height = glm::dot(diff, gradient);

    return height * t.x * t.y * t.z;
}

glm::vec2 ProcTerrainGen::noiseNormalVector(glm::vec2 v) {
    v += 0.01;
    glm::mat2 primes = glm::mat2{{126.1, 311.7}, {420.2, 1337.1}};

    return glm::normalize(glm::abs(glm::fract(glm::sin(primes * v))));
}

float ProcTerrainGen::linearInterp(float a, float b, float t) {
    return a * (1 - t) + b * t;
}

float ProcTerrainGen::cosineInterp(float a, float b, float t) {
    t = (1 - cos(t * PI)) * 0.5;

    return linearInterp(a, b, t);
}

float ProcTerrainGen::interpNoise(float x, float z) {
    float intX, fractX;
    fractX = modf(x, &intX);

    float intZ, fractZ;
    fractZ = modf(z, &intZ);

    float v1 = smoothNoise(intX, intZ);
    float v2 = smoothNoise(intX + 1, intZ);
    float v3 = smoothNoise(intX, intZ + 1);
    float v4 = smoothNoise(intX + 1, intZ + 1);

    float i1 = cosineInterp(v1, v2, fractX);
    float i2 = cosineInterp(v3, v4, fractX);

    return cosineInterp(i1, i2, fractZ);
}

float ProcTerrainGen::smoothNoise(float x, float z) {
    float corners = (noise(x - 1, z - 1) +
                     noise(x + 1, z - 1) +
                     noise(x - 1, z + 1) +
                     noise(x + 1, z + 1)) / 16;
    float sides = (noise(x - 1, z) +
                   noise(x + 1, z) +
                   noise(x, z - 1) +
                   noise(x, z + 1)) / 8;
    float center = noise(x, z) / 4;

    return corners + sides + center;
}

float ProcTerrainGen::noise(float x, float z) {
    float s = sin(glm::dot(glm::vec2(x, z), glm::vec2(127.1, 311.7))) * 43758.5453;

    return modf(s, nullptr);
}
