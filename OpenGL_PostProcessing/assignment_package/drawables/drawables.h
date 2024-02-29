#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include "utils.h"

enum BufferType {
    POSITION, NORMAL, UV,
    INDEX
};

class Drawable {
protected:
    // See MyGL's `glContext` member for more info
    QOpenGLFunctions_3_3_Core *glContext;
    // These four GLuints will serve as the CPU-side handles
    // to GPU-side arrays of vertex information, also known as
    // vertex buffers. bufferPosition will be the handle for
    // an array of vertex position data, bufferNormal will be
    // the handle for vertex normals, and bufferUV will be
    // the handle for vertex texture coordinates. bufferIndex
    // will be the handle for the ordering of vertices used to
    // build triangle primitives.
    GLuint bufferPosition;
    GLuint bufferNormal;
    GLuint bufferUV;
    GLuint bufferIndex;

    // We will store the number of indices that we send to our
    // index buffer. For example, if the index buffer was
    // {0, 1, 2, 0, 2, 3}, then indexBufferLength would be 6.
    // We need this information in order to tell
    // the glDrawElements function how many indices to read when
    // drawing our geometry.
    int indexBufferLength;

public:
    Drawable(QOpenGLFunctions_3_3_Core*);
    virtual ~Drawable();

    // Generate vertex position and triangle index data
    // for your mesh, then pass it to the GPU to be stored
    // in buffers.
    virtual void initializeAndBufferGeometryData() = 0;

    // Public function that lets other scopes call
    // glBindBuffer on each of the four buffers in Drawable
    void bindBuffer(BufferType t);

    int getIndexBufferLength() const;
};

class Mesh : public Drawable {
private:
    std::string objFilePath;

public:
    Mesh(QOpenGLFunctions_3_3_Core*, std::string objFilePath);
    ~Mesh();

    // A function that, for the Mesh class,
    // reads the OBJ file referred to by
    // `objFilePath` and buffers the data
    // to the GPU.
    // We have provided the code that reads
    // the OBJ file; you must write the code
    // that buffers it to the GPU.
    void initializeAndBufferGeometryData() override;

};

class Quad : public Drawable {
public:
    Quad(QOpenGLFunctions_3_3_Core*);
    ~Quad();

    // A function that initializes the vertex
    // coordinates for a quadrangle that covers
    // the entire screen in screen space,
    // then buffers that data to the GPU.
    void initializeAndBufferGeometryData() override;
};
