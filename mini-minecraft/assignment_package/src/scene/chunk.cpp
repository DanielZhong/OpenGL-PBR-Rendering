#include "chunk.h"


Chunk::Chunk(OpenGLContext *context) : Drawable(context), m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getLocalBlockAt(int x, int y, int z) const {
    return getLocalBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

BlockType Chunk::getNeighbors(int x, int y, int z, glm::vec4 dir) const
{
    BlockType block = EMPTY;

    int x_coord = x + (int) dir[0];
    int y_coord = y + (int) dir[1];
    int z_coord = z + (int) dir[2];

    if (y_coord == -1 || y_coord == 256) {
        return block;
    }

    std::tuple<int, Direction, int> xBound[2] = {std::make_tuple(-1, XNEG, 15), std::make_tuple(16, XPOS, 0)};
    for (int i = 0; i < 2; i++) {
        if (x_coord == std::get<0>(xBound[i])) {
            Direction dirc = std::get<1>(xBound[i]);
            Chunk *neighbor = m_neighbors.at(dirc);
            return neighbor != nullptr ? neighbor->getLocalBlockAt(std::get<2>(xBound[i]), y, z) : block;
        }
    }

    std::tuple<int, Direction, int> zBound[2] = {std::make_tuple(-1, ZNEG, 15),
                                                 std::make_tuple(16, ZPOS, 0)};
    for (int i = 0; i < 2; i++) {
        if (z_coord == std::get<0>(zBound[i])) {
            Direction dirc = std::get<1>(zBound[i]);
            Chunk *neighbor = m_neighbors.at(dirc);
            return neighbor != nullptr ? neighbor->getLocalBlockAt(x, y, std::get<2>(zBound[i])) : block;
        }
    }

    return getLocalBlockAt(x_coord, y_coord, z_coord);
}



void pushBuffer(std::vector<float> &buffer, const glm::vec4 &vec) {
    for (int i = 0; i < 4; i++) {
        buffer.push_back(vec[i]);
    }
}

void pushBuffer(std::vector<float> &buffer, const glm::vec3 &vec) {
    for (int i = 0; i < 3; i++) {
        buffer.push_back(vec[i]);
    }
}

void pushBuffer(std::vector<float> &buffer, const glm::vec2 &vec) {
    for (int i = 0; i < 2; i++) {
        buffer.push_back(vec[i]);
    }
}

glm::vec3 ComputeTangent(const Vertex &v0, const Vertex &v1, const Vertex &v2) {
    // Convert glm::vec4 to glm::vec3 by ignoring the w component
    glm::vec3 edge1 = glm::vec3(v1.pos) - glm::vec3(v0.pos);
    glm::vec3 edge2 = glm::vec3(v2.pos) - glm::vec3(v0.pos);
    glm::vec2 deltaUV1 = v1.uv - v0.uv;
    glm::vec2 deltaUV2 = v2.uv - v0.uv;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    glm::vec3 tangent;
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent = glm::normalize(tangent);

    return tangent;
}


glm::vec3 ComputeBitangent(const Vertex &v0, const Vertex &v1, const Vertex &v2, const glm::vec3 &tangent) {
    // Convert glm::vec4 to glm::vec3 for position vectors
    glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(v1.pos) - glm::vec3(v0.pos), glm::vec3(v2.pos) - glm::vec3(v0.pos)));
    glm::vec3 bitangent = glm::cross(normal, tangent);
    return glm::normalize(bitangent);
}


bool Chunk::checkBlockType(bool drawType, BlockType blockType) const {
    if (blockType == EMPTY) {
        return false;
    }

    if (!drawType && ChunkHelper::isTransparent(blockType)) {
        return false;
    }

    if (drawType && ChunkHelper::isOpaque(blockType)) {
        return false;
    }

    return true;
}


bool Chunk::checkNeighborBlock(bool drawType, BlockType neighborType) const {
    if (ChunkHelper::isOpaque(neighborType)) {
        return false;
    }

    if (drawType && neighborType == WATER) {
        return false;
    }

    return true;
}



//generate opaque chunk data
void Chunk::generateOpaData(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx)
{
    // init
    int nVertices = 0;

    std::vector<GLuint> faceIndices = {0, 1, 2, 0, 2, 3};

    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 256; y++) {
            for (int z = 0; z < 16; z++) {
                BlockType blockType = getLocalBlockAt(x, y, z);
                if (!checkBlockType(false, blockType)) {
                    continue;
                }

                for (const Face &face : ChunkHelper::Blocks[blockType]) {
                    BlockType neighbors = getNeighbors(x, y, z, face.normal);
                    if (!checkNeighborBlock(false, neighbors)) {
                        continue;
                    }

                    glm::vec3 tangent = ComputeTangent(face.vertices[0], face.vertices[1], face.vertices[2]);
                    glm::vec3 bitangent = ComputeBitangent(face.vertices[0], face.vertices[1], face.vertices[2], tangent);
                    for (const Vertex &v : face.vertices) {
                        pushBuffer(vertexVBOdata, v.pos + glm::vec4(x, y, z, 0));
                        pushBuffer(vertexVBOdata, face.normal);
                        pushBuffer(vertexVBOdata, ChunkHelper::getColor(blockType));
                        pushBuffer(vertexVBOdata, ChunkHelper::getUV(blockType, face.dir) + v.uv);
                        pushBuffer(vertexVBOdata, ChunkHelper::getAnimated(blockType));
                        pushBuffer(vertexVBOdata, tangent);
                        pushBuffer(vertexVBOdata, bitangent);
                    }

                    for (int index : faceIndices) {
                        idx.push_back(nVertices + index);
                    }

                    nVertices += 4;
                }
            }
        }
    }
}

//generate transparent chunk data
void Chunk::generateTransData(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx)
{
    // init
    int nVertices = 0;

    std::vector<GLuint> faceIndices = {0, 1, 2, 0, 2, 3};

    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 256; y++) {
            for (int z = 0; z < 16; z++) {
                BlockType blockType = getLocalBlockAt(x, y, z);
                if (!checkBlockType(true, blockType)) {
                    continue;
                }

                for (const Face &face : ChunkHelper::Blocks[blockType]) {
                    BlockType neighbors = getNeighbors(x, y, z, face.normal);
                    if (!checkNeighborBlock(true, neighbors)) {
                        continue;
                    }

                    glm::vec3 tangent = ComputeTangent(face.vertices[0], face.vertices[1], face.vertices[2]);
                    glm::vec3 bitangent = ComputeBitangent(face.vertices[0], face.vertices[1], face.vertices[2], tangent);
                    for (const Vertex &v : face.vertices) {
                        pushBuffer(vertexVBOdata, v.pos + glm::vec4(x, y, z, 0));
                        pushBuffer(vertexVBOdata, face.normal);
                        pushBuffer(vertexVBOdata, ChunkHelper::getColor(blockType));
                        pushBuffer(vertexVBOdata, ChunkHelper::getUV(blockType, face.dir) + v.uv);
                        pushBuffer(vertexVBOdata, ChunkHelper::getAnimated(blockType));
                        pushBuffer(vertexVBOdata, tangent);
                        pushBuffer(vertexVBOdata, bitangent);
                    }

                    for (int index : faceIndices) {
                        idx.push_back(nVertices + index);
                    }

                    nVertices += 4;
                }
            }
        }
    }
}

//pass to gpu
void Chunk::createOpaVBOdata(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx)
{
    indexCounts[INDEX] = idx.size();

    int bufferSize = vertexVBOdata.size();

    generateBuffer(INDEX);
    bindBuffer(INDEX);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCounts[INDEX] * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateBuffer(POSITION);
    bindBuffer(POSITION);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(NORMAL);
    bindBuffer(NORMAL);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(COLOR);
    bindBuffer(COLOR);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(UV);
    bindBuffer(UV);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(ANIMATED);
    bindBuffer(ANIMATED);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(TANGENT);
    bindBuffer(TANGENT);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(BITANGENT);
    bindBuffer(BITANGENT);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

}

void Chunk::createTransVBOdata(std::vector<float>& vertexVBOdata, std::vector<GLuint>& idx)
{
    indexCounts[INDEX_TRAN] = idx.size();

    int bufferSize = vertexVBOdata.size();

    generateBuffer(INDEX_TRAN);
    bindBuffer(INDEX_TRAN);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCounts[INDEX_TRAN] * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateBuffer(POSITION2);
    bindBuffer(POSITION2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(NORMAL2);
    bindBuffer(NORMAL2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(COLOR2);
    bindBuffer(COLOR2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(UV2);
    bindBuffer(UV2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(ANIMATED2);
    bindBuffer(ANIMATED2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(TANGENT2);
    bindBuffer(TANGENT2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

    generateBuffer(BITANGENT2);
    bindBuffer(BITANGENT2);
    mp_context->glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), vertexVBOdata.data(), GL_STATIC_DRAW);

}


void Chunk::createVBOdata() {
    std::vector<float> vertexVBOdata_opa, vertexVBOdata_trans;
    std::vector<GLuint> idx_opa, idx_trans;
    //generateOpaData(vertexVBOdata_opa, idx_opa);
    //createOpaVBOdata(vertexVBOdata_opa, idx_opa);
    generateTransData(vertexVBOdata_trans, idx_trans);
    createTransVBOdata(vertexVBOdata_trans, idx_trans);
    //setVBOCreated(true);
}

