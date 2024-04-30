#ifndef PROCTERRAINGEN_H
#define PROCTERRAINGEN_H


#include <math.h>
#include <glm_includes.h>
enum BiomeType : unsigned char {
  MOUNTAIN, GRASSLAND, DESERT, SNOWLAND
};


class ProcTerrainGen
{
public:
    ProcTerrainGen();
    static int getHeight(int x, int z);
    static float grasslands(float x, float z);
    static float mountains(float x, float z);
    static float caves(float x, float y, float z);
    static float snowlands(float x, float z);
    static float desert(float x, float z);

    static BiomeType getTerrainType(float t, float m);
    static float biomeInterp(glm::vec4 heights, glm::vec2 mt);

    static float worleyNoise(float x, float z);
    static glm::vec2 rand2(glm::vec2);
    static glm::vec3 rand3(glm::vec3);

    static float fbm(float x, float z, float persistence);
    static float fbm3D(float x, float y, float z, float persistence);
    static float perlinNoise(glm::vec2);
    static float perlinNoise3D(glm::vec3);
    static float surflet(glm::vec2, glm::vec2);
    static float surflet3D(glm::vec3, glm::vec3);
    static glm::vec2 noiseNormalVector(glm::vec2);

    static float interpNoise(float x, float z);
    static float smoothNoise(float x, float z);
    static float noise(float x, float z);

    static float cosineInterp(float a, float b, float t);
    static float linearInterp(float a, float b, float t);
};

#endif // PROCTERRAINGEN_H
