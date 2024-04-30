#include "blocktypeworker.h"
#include <QMutex>




BlockTypeWorker::BlockTypeWorker(Chunk* c,
                                 glm::ivec2 pos,
                                 Terrain& t) : c(c), m_pos(pos), t(t)
{
    this->setAutoDelete(true);
}

BlockTypeWorker::~BlockTypeWorker() {}

void BlockTypeWorker::run() {
    int offsetX = 4 * (ProcTerrainGen::perlinNoise(glm::vec2(m_pos.x + 101.f, m_pos.y + 1001.f) / 0.1f) + 3);
    int offsetY = 3 * (ProcTerrainGen::perlinNoise(glm::vec2(m_pos.x + 23.f, m_pos.y + 71.f) / 0.1f) + 2);
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            if(x % offsetX == 0 && z % offsetY == 0)
                generateChunkData(c, m_pos, x, z, true);
            else
                generateChunkData(c, m_pos, x, z, false);
        }
    }
    t.newChunkInserter(c);
}


void BlockTypeWorker::generateChunkData(Chunk *c, glm::ivec2 pos, int x, int z, bool asset) {
    int globalx = pos.x + x;
    int globalz = pos.y + z;
    glm::vec2 globalPos(globalx, globalz);


    int desert = ProcTerrainGen::desert(globalx / 30.f, globalz / 30.f);
    int mountain = ProcTerrainGen::mountains(globalx, globalz);
    int grassland = ProcTerrainGen::grasslands(globalx, globalz);
    int snowland = ProcTerrainGen::snowlands(globalx, globalz);

    float t = glm::smoothstep(0.15f, 0.65f, (ProcTerrainGen::perlinNoise(globalPos / 450.f) + 1) / 2);
    float m = glm::smoothstep(0.15f, 0.65f, (ProcTerrainGen::perlinNoise(glm::vec2(globalx + 453.3, globalz + 924.6) / 450.f) + 1) / 2);
    int height = ProcTerrainGen::biomeInterp(glm::vec4(snowland, grassland, mountain, desert), glm::vec2(m, t));

    float v = ProcTerrainGen::perlinNoise(globalPos * 0.1f);

    BiomeType biomeType = ProcTerrainGen::getTerrainType(t,m);

    c->setLocalBlockAt(x, 0, z, BEDROCK);


    for(int i = 1; i < 128; i++){
        float y = ProcTerrainGen::caves(globalx, i, globalz);
        if(i <= 60.f){
            c->setLocalBlockAt(x, i, z, LAVA);
        }
        else if (y > 0.f) {
            if(y > 0.3f && y < 0.5f) {
                c->setLocalBlockAt(x, i, z, GOLD_STONE);
            }
            else {
                c->setLocalBlockAt(x, i, z, STONE);
            }
        } else {
            c->setLocalBlockAt(x, i, z, EMPTY);
        }
    }

    c->setLocalBlockAt(x, 128, z, STONE);
    // setup all blocks
    for (int j = 129; j < 256; ++j) {
        if (biomeType == MOUNTAIN) {
            if (j <= height && j > 138) {
                c->setLocalBlockAt(x, j, z, STONE);
            }
            else if (j <= height && j <= 138) {
                c->setLocalBlockAt(x, j, z, DIRT);
            }
        } else if (biomeType == GRASSLAND) {
            if (j < height) {
                // set blocks under the top to dirt
                c->setLocalBlockAt(x, j, z, DIRT);
            } else if (j == height && height > 138) {
               // set the top of grasslans to grass
                    c->setLocalBlockAt(x, height, z, GRASS);
            }
        } else if (biomeType == SNOWLAND) {
            if (j < height) {
                c->setLocalBlockAt(x, j, z, DIRT);
            } else if (j == height && height > 138) {
                c->setLocalBlockAt(x, height, z, SNOW);
            }
        } else if (biomeType == DESERT) {
            if (j <= height) {
                c->setLocalBlockAt(x, j, z, SAND);
            } else {
                c->setLocalBlockAt(x, j, z, EMPTY);
            }
        }
    }
    // set the top of mountain to snow if the mountain's height >= 200
    if (biomeType == MOUNTAIN && height >= 190) {
        c->setLocalBlockAt(x, height, z, SNOW);
    }

    // set empty blocks within height 128 to 138 as water
    for (int j = 129; j < 138; j++) {
        if (c->getLocalBlockAt(x, j, z) == EMPTY) {
            c->setLocalBlockAt(x, j, z, WATER);
        }
    }

    if(v > 0.3f && asset == true && height > 138 && biomeType != MOUNTAIN && x >= 2 && z >= 2 && x <= 13 && z <= 13) {
        if(biomeType == GRASSLAND || biomeType == SNOWLAND) {
            if (v < 0.5) {
                for (int i = 0; i < 4; ++i) {
                    c->setLocalBlockAt(x, height + i + 1, z, WOOD);
                }

                for (int i = -1; i <= 1; ++i) {
                    for (int j = -1; j <= 1; ++j) {
                        c->setLocalBlockAt(x + i, height + 4, z + j, LEAF);
                    }
                }

                for (int i = -2; i <= 2; ++i) {
                    for (int j = -2; j <= 2; ++j) {
                        c->setLocalBlockAt(x + i, height + 5, z + j, LEAF);
                    }
                }

                for (int i = -1; i <= 1; ++i) {
                    for (int j = -1; j <= 1; ++j) {
                        c->setLocalBlockAt(x + i, height + 6, z + j, LEAF);
                    }
                }

                c->setLocalBlockAt(x, height + 7, z, LEAF);

            }
            else {
                for (int i = 0; i < 5; ++i) {
                    c->setLocalBlockAt(x, height + i + 1, z, WOOD);
                }

                for (int i = -1; i <= 1; ++i) {
                    for (int j = -1; j <= 1; ++j) {
                        c->setLocalBlockAt(x + i, height + 5, z + j, LEAF);
                    }
                }

                for (int i = -2; i <= 2; ++i) {
                    for (int j = -2; j <= 2; ++j) {
                        c->setLocalBlockAt(x + i, height + 6, z + j, LEAF);
                    }
                }

                for (int i = -1; i <= 1; ++i) {
                    for (int j = -1; j <= 1; ++j) {
                        c->setLocalBlockAt(x + i, height + 7, z + j, LEAF);
                    }
                }

                c->setLocalBlockAt(x, height + 8, z, LEAF);
            }
        }
    }


    if(v > 0.4f && asset == true && height > 138 && biomeType == DESERT) {
        c->setLocalBlockAt(x, height + 1, z, CACTUS);
        c->setLocalBlockAt(x, height + 2, z, CACTUS);
    }

    if(v > 0.4f && asset == true && height > 138 && biomeType == GRASSLAND && height < 150 && c->getLocalBlockAt(x, height + 1, z) == EMPTY) {
        c->setLocalBlockAt(x, height + 1, z, RED_FLOWER);
    }

    if(biomeType ==GRASSLAND && height > 138 && c->getLocalBlockAt(x, height + 1, z) == EMPTY) {
       if(v > 0.7f) {
            c->setLocalBlockAt(x, height + 1, z, GRASS_MID);
       } else if(v > 0.5f && v < 0.7f) {
           c->setLocalBlockAt(x, height + 1, z, GRASS_LONG);
       }

    }


}


