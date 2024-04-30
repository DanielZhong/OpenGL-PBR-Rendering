#include "vbowork.h"

VBOWork::VBOWork(Chunk* c, Terrain& t) : c(c), t(t), vbo() {
    vbo.owner = c;
}

void VBOWork::run() {
    //vbo.owner = c;
    c->generateOpaData(vbo.opaqueVtxVBOdata, vbo.opaqueIdx);
    c->generateTransData(vbo.transparentVtxVBOdata, vbo.transparentIdx);
    t.insertVBO(c, vbo);
}

