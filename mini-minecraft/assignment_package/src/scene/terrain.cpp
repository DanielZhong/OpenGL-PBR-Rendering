#include "terrain.h"
#include "cube.h"
#include <stdexcept>
#include <iostream>

#include "blocktypeworker.h"
#include "vbowork.h"

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_chunkVBOs(), m_generatedTerrain(), mp_context(context), mp_thd_pool(QThreadPool::globalInstance())
{}

Terrain::~Terrain() {}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

QSet<int64_t> Terrain::setChunkBound(float playerX, float playerZ, int half, int &minX,
                      int &maxX, int &minZ, int &maxZ) {
    int xFloor = static_cast<int>(glm::floor(playerX / 16.f));
    int zFloor = static_cast<int>(glm::floor(playerZ / 16.f));

    QSet<int64_t> result;

    minX = (xFloor - half) * 16;
    maxX = (xFloor + (half + 1)) * 16;
    minZ = (zFloor - half) * 16;
    maxZ = (zFloor + (half + 1)) * 16;

    for(int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            result.insert(toKey(x, z));
        }
    }

    return result;
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getGlobalBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                                  static_cast<unsigned int>(y),
                                  static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getGlobalBlockAt(glm::vec3 p) const {
    return getGlobalBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setGlobalBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                           static_cast<unsigned int>(y),
                           static_cast<unsigned int>(z - chunkOrigin.y),
                           t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context);
    m_chunksLock.lock();
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    m_chunksLock.unlock();
    return cPtr;
}

void Terrain::drawProximity(float player_x, float player_z, int half, ShaderProgram *shaderProgram) {
    int minX, maxX, minZ, maxZ;
    setChunkBound(player_x, player_z, half, minX, maxX, minZ, maxZ);
    draw(minX, maxX, minZ, maxZ, shaderProgram);

}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    for (int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            if (!hasChunkAt(x, z)) {
                continue;
            }

            uPtr<Chunk> &chunk = getChunkAt(x, z);
            if(chunk->elemCount(INDEX) < 0) {
                continue;
            }
            shaderProgram->setModelMatrix(glm::translate(glm::mat4(1.f), glm::vec3(x, 0, z))); // todo: remove
            shaderProgram->drawInterleaved(*chunk);

        }
    }
    for (int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            if (!hasChunkAt(x, z)) {
                continue;
            }

            uPtr<Chunk> &chunk = getChunkAt(x, z);
            if(chunk->elemCount(INDEX_TRAN) < 0) {
                continue;
            }
            shaderProgram->setModelMatrix(glm::translate(glm::mat4(1.f), glm::vec3(x, 0, z)));
            shaderProgram->drawTrans(*chunk);

        }
    }
}

void Terrain::spawmBlockWorker(Chunk* c, int cx, int cz) {
    mp_thd_pool->start(new BlockTypeWorker(c, glm::ivec2(cx, cz), *this));
}


void Terrain::insertVBO(Chunk* c, ChunkVBOData vbo) {
    m_VBOLock.lock();
    m_chunkVBOs[c] = vbo;
    m_VBOLock.unlock();
}

void Terrain::tryExpand(float player_x, float player_z, float pre_x, float pre_z, int half, bool init){

    int minX, maxX, minZ, maxZ;
    int pre_minX, pre_maxX, pre_minZ, pre_maxZ;
    QSet<int64_t> currZone = setChunkBound(player_x, player_z, half, minX, maxX, minZ, maxZ);
    QSet<int64_t> preZone = setChunkBound(pre_x, pre_z, half, pre_minX, pre_maxX, pre_minZ, pre_maxZ);
    QSet<int64_t> blockData;
    //QSet<int64_t> dataDelete;

    blockData.clear();

    if(init == false) {
        preZone.clear();
    }

    for (auto c : currZone) {
        if(!preZone.contains(c)) {
            blockData.insert(c);
        }
    }

    for (auto c : blockData) {
        auto coord = toCoords(c);
        if(!hasChunkAt(coord.x, coord.y)) {
            Chunk* c = instantiateChunkAt(coord.x, coord.y);
            spawmBlockWorker(c, coord.x, coord.y);
        }
    }

    //mp_thd_pool->waitForDone();

    blockData.clear();

    newChunkLock.lock();
    for (Chunk *chunk : newChunks) {
        mp_thd_pool->start(new VBOWork(chunk, *this));
    }
    newChunks.clear();
    newChunkLock.unlock();
    //mp_thd_pool->waitForDone();

    m_VBOLock.lock();
    for(auto& data : m_chunkVBOs) {
        data.first->createOpaVBOdata(data.second.opaqueVtxVBOdata, data.second.opaqueIdx);
        data.first->createTransVBOdata(data.second.transparentVtxVBOdata, data.second.transparentIdx);
    }
    m_chunkVBOs.clear();
    m_VBOLock.unlock();

}


void Terrain::instantiateChunks(int cx, int cz) {
    instantiateChunkAt(cx, cz);

    for (int x = cx; x < cx + 16; x++) {
        for (int z = cz; z < cz + 16; z++) {
            genPos(x, z);
        }
    }
}


void Terrain::genPos(int x, int z) {
    int h = floor(ProcTerrainGen::getHeight(x, z));
    h = h >= 129 ? h : 129;

    if(h <= 155) {
        for(int i = 129; i < h; ++i) {
            setGlobalBlockAt(x, i, z, DIRT);
        }

        setGlobalBlockAt(x, h, z, GRASS);

        if(h < 138) {
            for (int i = h + 1; i <= 138; ++i) {
                setGlobalBlockAt(x, i, z, WATER);
            }
        }
    }

    if(h > 155) {
        for(int i = 129; i < h; ++i) {
            setGlobalBlockAt(x, i, z, STONE);
        }

        setGlobalBlockAt(x, h, z, h > 200 ? SNOW : STONE);
    }

    setGlobalBlockAt(x, 0, z, BEDROCK);

    for(int i = 1; i < 128; i++){
        float y = ProcTerrainGen::caves(x, i, z);
        if(y > 0.f){
            if(i <= 25){
                setGlobalBlockAt(x, i, z, LAVA);
            }
            else{
                setGlobalBlockAt(x, i, z, EMPTY);
            }
        } else {
            setGlobalBlockAt(x, i, z, STONE);
        }
    }
}

BlockType Terrain::search(int x, int y, int z) {
     for (int i = -1; i <= 1; ++i) {
         for (int j = -1; j <= 1; ++j) {
             for (int k = 0; z <= 1; ++z) {
                 if(getGlobalBlockAt(x + i, y + k, z + j) == WATER || getGlobalBlockAt(x + i, y + k, z + j) == LAVA)
                     return getGlobalBlockAt(x + i, y + k, z + j);
             }
         }
     }
     return EMPTY;
}

void Terrain::blockInteraction(int x, int y, int z, BlockType t) {
    setGlobalBlockAt(x, y, z, t);
    int cx = static_cast<int>(glm::floor(x / 16.f)) * 16;
    int cz = static_cast<int>(glm::floor(z / 16.f)) * 16;
    const uPtr<Chunk> &chunk = getChunkAt(cx, cz);
    // ChunkVBOdata data = chunk->generateOpaData();
    ChunkVBOData vbo;
    chunk->generateTransData(vbo.transparentVtxVBOdata, vbo.transparentIdx);
    chunk->createTransVBOdata(vbo.transparentVtxVBOdata, vbo.transparentIdx);
    chunk->generateOpaData(vbo.opaqueVtxVBOdata, vbo.opaqueIdx);
    chunk->createOpaVBOdata(vbo.opaqueVtxVBOdata, vbo.opaqueIdx);
}

void Terrain::newChunkInserter(Chunk *c) {
    newChunkLock.lock();
    newChunks.insert(c);
    newChunkLock.unlock();
}





