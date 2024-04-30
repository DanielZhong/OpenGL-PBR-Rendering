#pragma once
#include "glm_includes.h"
#include <unordered_map>
#include <array>


//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, SNOW, LAVA, BEDROCK, SAND, WOOD, LEAF, CACTUS, RED_FLOWER, GRASS_MID, GRASS_LONG, GOLD_STONE
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};


struct Vertex {
    glm::vec4 pos;
    glm::vec2 uv;

    Vertex() : pos(), uv() {}
    Vertex(glm::vec4 pos, glm::vec2 uv) : pos(pos), uv(uv) {}

};

struct Face
{

    // represents a face of a cube
    Direction dir;
    glm::vec4 normal;
    std::array<Vertex, 4> vertices;

    Face(): dir(), normal(), vertices() {}
    Face(Direction direction, glm::vec4 normal, const Vertex &v1, const Vertex &v2, const Vertex &v3, const Vertex &v4)
        : dir(direction), normal(normal), vertices({v1, v2, v3, v4}) {}
};


class ChunkHelper {

private:
    static std::array<Face, 6> createFace();
    static std::array<Face, 6> createFace(std::array<glm::vec2, 6> Offsets);
    static std::array<Face, 6> createFace_flower_grass();
    static std::array<Face, 6> createFace_water();
    static std::array<Face, 6> createFace_cactus();


public:
    static std::unordered_map<BlockType, std::array<Face, 6>> Blocks;

    static std::unordered_map<BlockType, std::array<Face, 4>> Other_block;

    static bool isOpaque(BlockType type);

    static bool isTransparent(BlockType type);

    static glm::vec4 getColor(BlockType type);

    static glm::vec2 getUV(BlockType type, Direction dir);

    static glm::vec2 getAnimated(BlockType type);

};
