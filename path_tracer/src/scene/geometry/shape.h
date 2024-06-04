#pragma once

#include <globals.h>
#include <scene/materials/material.h>
#include <scene/transform.h>
#include <QString>
#include "glslwriting.h"
#include "openglcontext.h"

struct Material;


//Geometry is an abstract class since it contains a pure virtual function (i.e. a virtual function that is set to 0)
class Shape {
public:
    Shape();
    Shape(const Material &m);

    virtual ~Shape(){}

    virtual QString toGLSL(int ID) const = 0;

    QString writeMembers(int ID) const;

    Transform transform;
    uPtr<Material> material;
};

class Box : public Shape {
private:
    glm::vec3 minCorner, maxCorner;
public:
    Box();
    Box(glm::vec3, glm::vec3, const Material &m);
    Box(const Material &m);
    QString toGLSL(int ID) const override;
};

class Sphere : public Shape {
private:
    glm::vec3 pos;
    float radius;
public:
    Sphere(const Material &m);
    Sphere();
    QString toGLSL(int ID) const override;
};

class RectangleShape : public Shape {
private:
    glm::vec3 pos, nor;
    glm::vec2 halfSideLengths;
public:
    RectangleShape(const Material &m);
    RectangleShape();
    RectangleShape(glm::vec3, glm::vec3, glm::vec2, const Material &m);
    QString toGLSL(int ID) const override;
};

class Mesh;

class Triangle {
private:
    std::array<glm::vec3, 3> pos;
    std::array<glm::vec3, 3> nor;
    std::array<glm::vec3, 3> uv;
    int index_in_mesh; // What index in the mesh's vector<Triangles> does this sit at?

public:
    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, int idx);
    QString toGLSL() const;

    glm::vec3 operator[](unsigned int) const;

    friend class Mesh;
};

class TextureTriangleStorage;

class Mesh : public Shape {
private:
    std::vector<Triangle> triangles;

    // Shader has an array of sampler2Ds
    // to read each Mesh's triangles.
    // Each Mesh knows which sampler2D reads its data
    int triangleSamplerIndex;
    int triangleStorageSideLen;
    uPtr<TextureTriangleStorage> trianglesAsTexture;

public:
    static unsigned int nextLowestSamplerIndex;

    Mesh(const Material &m, OpenGLContext *context);
    Mesh(OpenGLContext *context, int triSamplerIndex, int triTexSlot);

    void LoadOBJ(const QString &filename, const QString &local_path, int triangle_mesh_id);
    QString toGLSL(int ID) const override;
    unsigned int numTris() const;
    void computeStorageDimensions(int *w, int *h) const;

    friend class Scene;
    friend class TextureTriangleStorage;
    friend class MyGL;
};
