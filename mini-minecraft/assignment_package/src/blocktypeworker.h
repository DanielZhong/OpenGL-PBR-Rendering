#pragma once

#include <QRunnable>
#include <QMutex>
#include <QThreadPool>
#include <scene/terrain.h>
#include "procterraingen.h"

class Terrain;
class Chunk;

class BlockTypeWorker : public QRunnable
{
public:
    BlockTypeWorker(Chunk* c,
                    glm::ivec2 pos,
                    Terrain& t);
    ~BlockTypeWorker();
    void run() override;
    //noise chunk generate
    void generateChunkData(Chunk* c, glm::ivec2 pos, int x, int z, bool asset);
private:
    glm::ivec2 m_pos;
    Chunk* c;
    Terrain& t;
};


