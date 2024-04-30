#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "chunk.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include "shaderprogram.h"
#include "cube.h"
#include "procterraingen.h"

#include <QThreadPool>
#include <QMutex>

#define DRAW_RADIUS 2
#define GEN_RADIUS 3




//using namespace std;

struct ChunkVBOData {
    Chunk* owner;
    std::vector<float> opaqueVtxVBOdata;
    std::vector<GLuint> opaqueIdx;
    std::vector<float> transparentVtxVBOdata;
    std::vector<GLuint> transparentIdx;
    ChunkVBOData() : owner(), opaqueVtxVBOdata(), opaqueIdx(), transparentVtxVBOdata(), transparentIdx() {}
};

// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.
class Terrain {
private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.
    std::unordered_map<int64_t, uPtr<Chunk>> m_chunks;
    QMutex m_chunksLock;

    std::unordered_map<Chunk*, ChunkVBOData> m_chunkVBOs;
    QMutex m_VBOLock;



    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    std::unordered_set<int64_t> m_generatedTerrain;

    // TODO: DELETE ALL REFERENCES TO m_geomCube AS YOU WILL NOT USE
    // IT IN YOUR FINAL PROGRAM!
    // The instance of a unit cube we can use to render any cube.
    // Presently, Terrain::draw renders one instance of this cube
    // for every non-EMPTY block within its Chunks. This is horribly
    // inefficient, and will cause your game to run very slowly until
    // milestone 1's Chunk VBO setup is completed.

    // Set this to "true" whenever you modify the blocks
    // in your terrain. NOT NEEDED ONCE MILESTONE 1's CHUNKING
    // IS IMPLEMENTED.
    //used to store chunk that already created Blockdata by thread
    QMutex newChunkLock;
    QMutex VBOLock;
    QMutex zoneLock;

    OpenGLContext* mp_context;
    QThreadPool* mp_thd_pool;


public:
    Terrain(OpenGLContext *context);
    ~Terrain();

    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    // Returns a pointer to the created Chunk.
    Chunk* instantiateChunkAt(int x, int z);
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getGlobalBlockAt(int x, int y, int z) const;
    BlockType getGlobalBlockAt(glm::vec3 p) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setGlobalBlockAt(int x, int y, int z, BlockType t);

    void insertVBO(Chunk* c, ChunkVBOData vbo);


    void draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram);
    void drawProximity(float playerX, float playerZ, int half, ShaderProgram *shaderProgram);

    QSet<int64_t> setChunkBound(float playerX, float playerZ, int half, int &minX,
                                int &maxX, int &minZ, int &maxZ);


    void instantiateChunks(int X, int Z);
    void tryExpand(float currX, float currZ, float preX, float preZ, int half, bool init);

    //added
    void genPos(int x, int z);

    //init terrain then doing other work
    void initTerrain(glm::vec3 playerPos);

    void blockInteraction(int x, int y, int z, BlockType t);

    //access thread
    void spawmBlockWorker(Chunk* c, int x, int z);

    //access thread generate and buffer VBO data
    //void spawmVBOWorker(Chunk* c, ChunkVBOData vbo);
    void spawmVBOWorker(Chunk* c, ChunkVBOData& vbo);

    //store block type data
    std::unordered_set<Chunk*> newChunks = std::unordered_set<Chunk*>();

    BlockType search(int x, int y, int z);

    //used to pass blocktypeworker to shared resources
    void newChunkInserter(Chunk* c);

};
