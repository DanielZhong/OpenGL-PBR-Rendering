#include "vboworker.h"

VBOWorker::VBOWorker(Terrain& terrain, Chunk* target)
    : m_terrain(terrain),
      m_target(target) {}

void VBOWorker::run() {
    ChunkVBOData vboData{m_target, {}, {}, {}, {}};
    m_target->generateOpaData(vboData.opaqueVtxVBOdata, vboData.opaqueIdx);
    m_target->generateTransData(vboData.transparentVtxVBOdata, vboData.transparentIdx);
    m_terrain.insertChunkVBOData(vboData);
}
