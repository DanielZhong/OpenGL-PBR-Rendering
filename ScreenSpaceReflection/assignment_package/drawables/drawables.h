#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include "utils.h"

enum class BufferType {
    INDEX = 0,
    POSITION = 1,
    NORMAL = 2,
    TANGENT = 3,
    BITANGENT = 4,
    UV = 5
};

class Drawable {
protected:
    // See MyGL's `glContext` member for more info
    QOpenGLFunctions_3_3_Core *glContext;

    std::unordered_map<BufferType, GLuint> bufferHandles;

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
    virtual void destroy();

    void generateBuffer(BufferType t);
    void bindBuffer(BufferType t);
    bool hasBuffer(BufferType t) const;

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

class Cube : public Drawable {
public:
    Cube(QOpenGLFunctions_3_3_Core*);
    ~Cube();
    void initializeAndBufferGeometryData() override;
};
