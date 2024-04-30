#pragma once
#include <openglcontext.h>
#include <glm_includes.h>
#include <unordered_map>

#define dict std::unordered_map

enum BufferType : unsigned char {
    INDEX, INDEX_TRAN, INDEX_QUAD,
    POSITION, NORMAL, COLOR, UV,
    POSITION2, NORMAL2, COLOR2, UV2,
    POSITION3, NORMAL3, COLOR3, UV3,
    INTERLEAVED,
    INSTANCED_OFFSET, ANIMATED, ANIMATED2,
    TANGENT, TANGENT2, BITANGENT, BITANGENT2,
};

//This defines a class which can be rendered by our shader program.
//Make any geometry a subclass of ShaderProgram::Drawable in order to render it with the ShaderProgram class.
class Drawable
{
protected:
    dict<BufferType, GLuint> bufHandles;
    dict<BufferType, bool> bufGenerated;
    // The length of the index buffer indicated by the key.
    // Unless you have more than one index buffer, this map
    // will have just one key-value pair.
    dict<BufferType, int> indexCounts;

    OpenGLContext* mp_context; // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                               // we need to pass our OpenGL context to the Drawable in order to call GL functions
                               // from within this class.

public:
    Drawable(OpenGLContext* context);
    virtual ~Drawable();

    virtual void createVBOdata() = 0; // To be implemented by subclasses. Populates the VBOs of the Drawable.
    virtual void destroyVBOdata(); // Frees the VBOs of the Drawable.

    // Getter functions for various GL data
    virtual GLenum drawMode();
    int elemCount(BufferType);

    // Call these functions when you want to call glGenBuffers on the buffers stored in the Drawable
    // These will properly set the values of idxBound etc. which need to be checked in ShaderProgram::draw()
    void generateBuffer(BufferType buf);

    bool bindBuffer(BufferType buf);
};


// A subclass of Drawable that enables the base code to render duplicates of
// the Terrain class's Cube member variable via OpenGL's instanced rendering.
// You will not have need for this class when completing the base requirements
// for Mini Minecraft, but you might consider using instanced rendering for
// some of the milestone 3 ideas.
class InstancedDrawable : public Drawable {
protected:
    int m_numInstances;

public:
    InstancedDrawable(OpenGLContext* mp_context);
    virtual ~InstancedDrawable();
    int instanceCount() const;

    void generateOffsetBuf();
    bool bindOffsetBuf();
    void clearOffsetBuf();
    void clearColorBuf();

    virtual void createInstancedVBOdata(std::vector<glm::vec3> &offsets, std::vector<glm::vec3> &colors) = 0;
};

class Quad : public Drawable {
public:
    Quad(OpenGLContext* context);
    ~Quad();

    // A function that initializes the vertex
    // coordinates for a quadrangle that covers
    // the entire screen in screen space,
    // then buffers that data to the GPU.
    void createVBOdata() override;
};

