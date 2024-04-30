#ifndef VBOWORK_H
#define VBOWORK_H

#include <QRunnable>
#include <QMutex>
#include "scene/terrain.h"
#include "scene/chunk.h"

class Terrain;
class Chunk;

class VBOWork : public QRunnable
{
public:
    VBOWork(Chunk* c, Terrain& t);
    void run() override;
private:
    Chunk* c;
    Terrain& t;
    ChunkVBOData vbo;
};

#endif // VBOWORK_H
