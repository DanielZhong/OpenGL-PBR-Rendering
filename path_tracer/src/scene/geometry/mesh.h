#if 0
#pragma once

#include "shape.h"
#include <QList>

class Triangle
{
public:
    Triangle(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3);
    Triangle(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec3 &n1, const glm::vec3 &n2, const glm::vec3 &n3);
    Triangle(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const glm::vec3 &n1, const glm::vec3 &n2, const glm::vec3 &n3, const glm::vec2 &t1, const glm::vec2 &t2, const glm::vec2 &t3);


    Point2f GetUVCoordinates(const Point3f &point) const;
    void ComputeTBN(const Point3f& P, Normal3f* nor, Vector3f* tan, Vector3f* bit) const;
    void ComputeTriangleTBN(const Point3f& P, Normal3f* nor, Vector3f* tan, Vector3f* bit, const Point2f& uv) const;

    Point3f points[3];
    Normal3f normals[3];
    Point2f uvs[3];
    Normal3f planeNormal;
};

//A mesh just holds a collection of triangles against which one can test intersections.
//Its primary purpose is to store VBOs for rendering the triangles in OpenGL.
class Mesh : public Shape
{
public:
    void LoadOBJ(const QString &filename, const QString &local_path);

    QString toGLSL(int ID) const override;

private:
    QList<Triangle*> faces;
};




#endif

