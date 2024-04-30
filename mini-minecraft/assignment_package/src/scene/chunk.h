#pragma once
#include "smartpointerhelp.h"
#include "drawable.h"
#include "glm_includes.h"
#include "chunkhelper.h"
#include <array>
#include <unordered_map>
#include <cstddef>


// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    BlockType getNeighbors(int x, int y, int z, glm::vec4 dir) const;

public:
    Chunk(OpenGLContext *context);
    BlockType getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getLocalBlockAt(int x, int y, int z) const;
    void setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    virtual void createVBOdata() override;
    void generateTransData(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx);
    void generateOpaData(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx);
    void createTransVBOdata(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx);
    void createOpaVBOdata(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx);
    int getAllNeighbors() const;
    bool checkBlockType(bool drawType, BlockType blockType) const;
    bool checkNeighborBlock(bool drawType, BlockType neighborType) const;

};
