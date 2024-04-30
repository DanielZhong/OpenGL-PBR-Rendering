#include "chunkhelper.h"

std::array<Face, 6> ChunkHelper::createFace()
{
    return createFace({glm::vec2(0.f), glm::vec2(0.f),
                       glm::vec2(0.f), glm::vec2(0.f),
                       glm::vec2(0.f), glm::vec2(0.f)});
};


std::array<Face, 6> ChunkHelper::createFace(std::array<glm::vec2, 6> Offsets) {
    float len = 1.f /16.f;
    return {
        Face(XPOS, glm::vec4( 1,  0,  0, 0),
             Vertex(glm::vec4(1, 0, 1, 1), Offsets[0]),
             Vertex(glm::vec4(1, 0, 0, 1), Offsets[0] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[0] + glm::vec2(len, len)),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[0] + glm::vec2(0, len))),
        Face(XNEG, glm::vec4(-1,  0,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[1]),
             Vertex(glm::vec4(0, 0, 1, 1), Offsets[1] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[1] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[1] + glm::vec2(0, len))),
        Face(YPOS, glm::vec4( 0,  1,  0, 0),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[2]),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[2] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[2] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[2] + glm::vec2(0, len))),
        Face(YNEG, glm::vec4( 0, -1,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3]),
             Vertex(glm::vec4(1, 0, 0, 1), Offsets[3] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 0, 1, 1), Offsets[3] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 1, 1), Offsets[3] + glm::vec2(0, len))),
        Face(ZPOS, glm::vec4( 0,  0,  1, 0),
             Vertex(glm::vec4(0, 0, 1, 1), Offsets[4]),
             Vertex(glm::vec4(1, 0, 1, 1), Offsets[4] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[4] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[4] + glm::vec2(0., len))),
        Face(ZNEG, glm::vec4( 0,  0, -1, 0),
             Vertex(glm::vec4(1, 0, 0, 1), Offsets[5]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[5] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[5] + glm::vec2(len, len)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[5] + glm::vec2(0., len)))
    };
};

std::array<Face, 6> ChunkHelper::createFace_flower_grass() {
    std::array<glm::vec2, 6> Offsets = {glm::vec2(0.f), glm::vec2(0.f),
                                        glm::vec2(0.f), glm::vec2(0.f),
                                        glm::vec2(0.f), glm::vec2(0.f)};
    float len = 1.f / 16.f;
    return {
        Face(XPOS, glm::vec4( 1,  0,  0, 0),
             Vertex(glm::vec4(0.499f, 0, 1, 1), Offsets[0]),
             Vertex(glm::vec4(0.499f, 0, 0, 1), Offsets[0] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0.499f, 0.5f, 0, 1), Offsets[0] + glm::vec2(len, len)),
             Vertex(glm::vec4(0.499f, 0.5f, 1, 1), Offsets[0] + glm::vec2(0, len))),
        Face(XNEG, glm::vec4(-1,  0,  0, 0),
             Vertex(glm::vec4(0.5f, 0, 0, 1), Offsets[1]),
             Vertex(glm::vec4(0.5f, 0, 1, 1), Offsets[1] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0.5f, 1, 1, 1), Offsets[1] + glm::vec2(len, len)),
             Vertex(glm::vec4(0.5f, 1, 0, 1), Offsets[1] + glm::vec2(0, len))),
        Face(YPOS, glm::vec4( 0,  0,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[2]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[2] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[2] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[2] + glm::vec2(0, len))),
        Face(YNEG, glm::vec4( 0,  0,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3] + glm::vec2(0, len))),
        Face(ZPOS, glm::vec4( 0,  0,  1, 0),
             Vertex(glm::vec4(0, 0, 0.499f, 1), Offsets[4]),
             Vertex(glm::vec4(1, 0, 0.499f, 1), Offsets[4] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 0.499f, 1), Offsets[4] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 0.499f, 1), Offsets[4] + glm::vec2(0., len))),
        Face(ZNEG, glm::vec4( 0,  0, -1, 0),
             Vertex(glm::vec4(1, 0, 0.5f, 1), Offsets[5]),
             Vertex(glm::vec4(0, 0, 0.5f, 1), Offsets[5] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0.5f, 0.5f, 1), Offsets[5] + glm::vec2(len, len)),
             Vertex(glm::vec4(1, 0.5f, 0.5f, 1), Offsets[5] + glm::vec2(0., len)))
    };
};

std::array<Face, 6> ChunkHelper::createFace_water() {
    std::array<glm::vec2, 6> Offsets = {glm::vec2(0.f), glm::vec2(0.f),
                                        glm::vec2(0.f), glm::vec2(0.f),
                                        glm::vec2(0.f), glm::vec2(0.f)};
    float len = 1.f /16.f;
    return {
        Face(XPOS, glm::vec4( 1,  0,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[0]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[0] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[0] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[0] + glm::vec2(0, len))),
        Face(XNEG, glm::vec4(-1,  0,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[1]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[1] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[1] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[1] + glm::vec2(0, len))),
        Face(YPOS, glm::vec4( 0,  1,  0, 0),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[2]),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[2] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[2] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[2] + glm::vec2(0, len))),
        Face(YNEG, glm::vec4( 0, -1,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3]),
             Vertex(glm::vec4(1, 0, 0, 1), Offsets[3] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 0, 1, 1), Offsets[3] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 1, 1), Offsets[3] + glm::vec2(0, len))),
        Face(ZPOS, glm::vec4( 0,  0,  1, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[4]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[4] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[4] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[4] + glm::vec2(0., len))),
        Face(ZNEG, glm::vec4( 0,  0, -1, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[5]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[5] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[5] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[5] + glm::vec2(0., len)))
    };
}

std::array<Face, 6> ChunkHelper::createFace_cactus() {
    std::array<glm::vec2, 6> Offsets = {glm::vec2(0.f), glm::vec2(0.f),
                                        glm::vec2(0.f), glm::vec2(0.f),
                                        glm::vec2(0.f), glm::vec2(0.f)};
    float len = 1.f /16.f;
    return {
        Face(XPOS, glm::vec4( 1,  0,  0, 0),
             Vertex(glm::vec4(1, 0, 1, 1), Offsets[0]),
             Vertex(glm::vec4(1, 0, 0, 1), Offsets[0] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[0] + glm::vec2(len, len)),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[0] + glm::vec2(0, len))),
        Face(XNEG, glm::vec4(-1,  0,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[1]),
             Vertex(glm::vec4(0, 0, 1, 1), Offsets[1] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[1] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[1] + glm::vec2(0, len))),
        Face(YPOS, glm::vec4( 0,  1,  0, 0),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[2]),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[2] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[2] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[2] + glm::vec2(0, len))),
        Face(YNEG, glm::vec4( 0, -1,  0, 0),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[3] + glm::vec2(0, len))),
        Face(ZPOS, glm::vec4( 0,  0,  1, 0),
             Vertex(glm::vec4(0, 0, 1, 1), Offsets[4]),
             Vertex(glm::vec4(1, 0, 1, 1), Offsets[4] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(1, 1, 1, 1), Offsets[4] + glm::vec2(len, len)),
             Vertex(glm::vec4(0, 1, 1, 1), Offsets[4] + glm::vec2(0., len))),
        Face(ZNEG, glm::vec4( 0,  0, -1, 0),
             Vertex(glm::vec4(1, 0, 0, 1), Offsets[5]),
             Vertex(glm::vec4(0, 0, 0, 1), Offsets[5] + glm::vec2(len, 0.)),
             Vertex(glm::vec4(0, 1, 0, 1), Offsets[5] + glm::vec2(len, len)),
             Vertex(glm::vec4(1, 1, 0, 1), Offsets[5] + glm::vec2(0., len)))
    };
}


//add here for opaque
bool ChunkHelper::isOpaque(BlockType type) {
    return !isTransparent(type);
}

//add here for transparent
bool ChunkHelper::isTransparent(BlockType type) {
    return (type == EMPTY || type == WATER || type == RED_FLOWER || type == GRASS_LONG || type == GRASS_MID || type == CACTUS);
}


glm::vec2 ChunkHelper::getAnimated(BlockType type) {
    if (type == WATER) {
        return glm::vec2(1.f);
    } else if (type == LAVA) {
        return glm::vec2(2.f);
    } else {
        return glm::vec2(0.f);
    }
}


glm::vec4 ChunkHelper::getColor(BlockType type)
{
    glm::vec4 color = glm::vec4();
    switch (type) {
    case GRASS:
        color = glm::vec4(95.f, 159.f, 53.f, 255.f) / 255.f;
        break;
    case DIRT:
        color =  glm::vec4(121.f, 85.f, 58.f, 255.f) / 255.f;
        break;
    case STONE:
        color =  glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        break;
    case WATER:
        color = glm::vec4(0.f, 0.f, 0.75f, 1.f);
        break;
    case SNOW:
        color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        break;
    default:
        color = glm::vec4(1.f, 0.f, 1.f, 1.f);
        break;
    }

    return color;
}

glm::vec2 ChunkHelper::getUV(BlockType type, Direction dir)
{
    glm::vec2 uv;

    switch(type) {
    case GRASS:
        if (dir == YPOS) {
            // grass top
            uv = glm::vec2(8, 13);
        } else if (dir == YNEG) {
            // grass bottom
            uv = glm::vec2(2, 15);
        } else {
            // grass side
            uv = glm::vec2(3, 15);
        }
        break;
    case WOOD:
        if(dir == YPOS) {
            uv = glm::vec2(5, 14);
        }
        else if(dir == YNEG) {
            uv = glm::vec2(5, 14);
        }
        else {
            uv = glm::vec2(4, 14);
        }
        break;
    case LEAF:
        uv = glm::vec2(5, 12);
        break;
    case DIRT:
        uv = glm::vec2(2, 15);
        break;
    case STONE:
        uv = glm::vec2(1, 15);
        break;
    case WATER:
        uv = glm::vec2(13, 3);
        break;
    case LAVA:
        uv = glm::vec2(13, 1);
        break;
    case SNOW:
        uv = glm::vec2(2, 11);
        break;
    case BEDROCK:
        uv = glm::vec2(1, 14);
        break;
    case SAND:
        uv = glm::vec2(0, 4);
        break;
    case CACTUS:
        uv = glm::vec2(6, 11);
        break;
    case RED_FLOWER:
        uv = glm::vec2(12, 15);
        break;
    case GRASS_MID:
        uv = glm::vec2(10, 10);
        break;
    case GRASS_LONG:
        uv = glm::vec2(12, 10);
        break;
    case GOLD_STONE:
        uv = glm::vec2(0, 13);
        break;
    default:
        // debug purple
        uv = glm::vec2(7, 1);
        break;
    }

    uv /= 16.f;

    return uv;
}



std::unordered_map<BlockType, std::array<Face, 6>> ChunkHelper::Blocks = {
        {{GRASS,  ChunkHelper::createFace()},
        {DIRT ,  ChunkHelper::createFace()},
        {STONE,  ChunkHelper::createFace()},
        {WATER,  ChunkHelper::createFace_water()},
        {SNOW,  ChunkHelper::createFace()},
        {LAVA,  ChunkHelper::createFace_water()},
        {BEDROCK,  ChunkHelper::createFace()},
        {SAND,  ChunkHelper::createFace()},
        {WOOD,  ChunkHelper::createFace()},
        {LEAF,  ChunkHelper::createFace()},
        {RED_FLOWER,  ChunkHelper::createFace_flower_grass()},
        {CACTUS,   ChunkHelper::createFace_cactus()},
        {GRASS_MID,  ChunkHelper::createFace_flower_grass()},
        {GRASS_LONG,  ChunkHelper::createFace_flower_grass()},
        {GOLD_STONE,  ChunkHelper::createFace()}
    }
};
