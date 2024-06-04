#include "shape.h"
#include <QDateTime>
#include <tinyobj/tiny_obj_loader.h>
#include <cmath>

Shape::Shape(const Material &m)
    : transform(), material(mkU<Material>(m))
{}

Shape::Shape()
    : transform(), material()
{}

Box::Box(const Material &m)
    : Shape(m), minCorner(-0.5, -0.5, -0.5), maxCorner(0.5, 0.5, 0.5)
{}

Sphere::Sphere(const Material &m)
    : Shape(m), pos(0,0,0), radius(1.)
{}

RectangleShape::RectangleShape(const Material &m)
    : Shape(m), pos(0,0,0), nor(0,0,1), halfSideLengths(0.5, 0.5)
{}

Box::Box()
    : Shape(), minCorner(-0.5, -0.5, -0.5), maxCorner(0.5, 0.5, 0.5)
{}

Box::Box(glm::vec3 minCorner, glm::vec3 maxCorner, const Material &m)
    : Shape(m), minCorner(minCorner), maxCorner(maxCorner)
{}

Sphere::Sphere()
    : Shape(), pos(0,0,0), radius(1.)
{}

RectangleShape::RectangleShape()
    : Shape(), pos(0,0,0), nor(0,0,1), halfSideLengths(0.5, 0.5)
{}

RectangleShape::RectangleShape(glm::vec3 pos,
                               glm::vec3 nor,
                               glm::vec2 halfSideLens,
                               const Material &m)
    : Shape(m), pos(pos), nor(nor), halfSideLengths(halfSideLens)
{}


Mesh::Mesh(const Material &m, OpenGLContext *context)
    : Shape(m), triangles(),
      triangleSamplerIndex(-1),
      triangleStorageSideLen(-1),
      trianglesAsTexture(mkU<TextureTriangleStorage>(context, -1, -1, this))
{}

Mesh::Mesh(OpenGLContext *context, int triSamplerIndex, int triTexSlot)
    : Shape(), triangles(),
      triangleSamplerIndex(triSamplerIndex),
      triangleStorageSideLen(-1),
      trianglesAsTexture(mkU<TextureTriangleStorage>(context, -1, triTexSlot, this))
{}

Triangle::Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, int idx)
    : pos{p1, p2, p3}, nor(), uv(), index_in_mesh(idx)
{}


QString Shape::writeMembers(int ID) const {
    return writeTransform(transform) + ", " +
           writeInt(ID) + ", " +
           material->toGLSL();
}


QString Sphere::toGLSL(int ID) const {
    return "Sphere(" + writeVec3(pos) + ", " +
            writeFloat(radius) + ", " +
            writeMembers(ID) + ")";
}

QString Box::toGLSL(int ID) const {
    return "Box(" + writeVec3(minCorner) + ", " +
            writeVec3(maxCorner) + ", " +
            writeMembers(ID) + ")";
}

QString RectangleShape::toGLSL(int ID) const {
    return "Rectangle(" + writeVec3(pos) + ", " +
            writeVec3(nor) + ", " +
            writeVec2(halfSideLengths) + ", " +
            writeMembers(ID) + ")";
}

QString Mesh::toGLSL(int ID) const {
    return "Mesh(" + writeInt(triangleSamplerIndex) + ", "
            + writeInt(triangleStorageSideLen) + ", "
            + writeInt(triangles.size()) + ", "
            + writeMembers(ID) + ")";
}


glm::vec3 Triangle::operator[](unsigned int i) const {
    if(i < 3) {
        return pos[i];
    }
    else if(i < 6) {
        return nor[i - 3];
    }
    else {
        return uv[i - 6];
    }
}

QString Triangle::toGLSL() const {
#if 0
    return "Triangle(vec3[3](" + writeVec3(pos[0]) +
            ", " + writeVec3(pos[1]) +
            ", " + writeVec3(pos[2]) + "), " +
            "vec3[3](" + writeVec3(nor[0]) +
            ", " + writeVec3(nor[1]) +
            ", " + writeVec3(nor[2]) + "), " +
            "vec2[3](" + writeVec2(uv[0]) +
            ", " + writeVec2(uv[1]) +
            ", " + writeVec2(uv[2]) + "), " +
            writeInt(mesh_id) + ")";
#endif
}

unsigned int Mesh::numTris() const {
    return triangles.size();
}

void Mesh::computeStorageDimensions(int *w, int *h) const {
    int numPixels = 9 * triangles.size();
    int twoPower = glm::ceil(log2f(numPixels));
    if(twoPower % 2 == 1) {
        twoPower += 1;
    }
    int totalPixels = powf(2, twoPower);
    *w = *h = sqrtf(totalPixels);
#if 0
    // Each triangle is 9 vec3s
    // Each pixel stores 1 vec3
    // so need 9 * numTris to store entire mesh's triangles
    int numPixels = 9 * triangles.size();
    // 2 to what power gets us at least numPixels?
    float logf = log2f(numPixels);
    int log = glm::ceil(logf);
    // Find the min side length of a square image that can store
    // at least the number of vec3s needed to represent all our triangles.
    // Also convert that side length to the next highest power of 2.
    int minWidth = glm::ceil(glm::sqrt((float)(log)));
    logf = glm::ceil(log2f(minWidth));

    *w = *h = glm::pow(2.f, logf);
#endif
}

unsigned int Mesh::nextLowestSamplerIndex = 0;

void Mesh::LoadOBJ(const QString &filename, const QString &local_path, int triangle_mesh_id)
{
    QString filepath = local_path; filepath.append(filename);
    std::vector<tinyobj::shape_t> shapes; std::vector<tinyobj::material_t> materials;
    std::string errors = tinyobj::LoadObj(shapes, materials, filepath.toStdString().c_str());
//    std::cout << errors << std::endl;
    if(errors.size() == 0)
    {
        int nextTriangleIndex = 0;
        //Read the information from the vector of shape_ts
        for(unsigned int i = 0; i < shapes.size(); i++)
        {
            std::vector<float> &positions = shapes[i].mesh.positions;
            std::vector<float> &normals = shapes[i].mesh.normals;
            std::vector<float> &uvs = shapes[i].mesh.texcoords;
            std::vector<unsigned int> &indices = shapes[i].mesh.indices;
            for(unsigned int j = 0; j < indices.size(); j += 3)
            {
                glm::vec3 p1(positions[indices[j]*3], positions[indices[j]*3+1], positions[indices[j]*3+2]);
                glm::vec3 p2(positions[indices[j+1]*3], positions[indices[j+1]*3+1], positions[indices[j+1]*3+2]);
                glm::vec3 p3(positions[indices[j+2]*3], positions[indices[j+2]*3+1], positions[indices[j+2]*3+2]);

                Triangle t = Triangle(p1, p2, p3, nextTriangleIndex++);
//                t.mesh_id = triangle_mesh_id;
                if(normals.size() > 0)
                {
                    glm::vec3 n1(normals[indices[j]*3], normals[indices[j]*3+1], normals[indices[j]*3+2]);
                    glm::vec3 n2(normals[indices[j+1]*3], normals[indices[j+1]*3+1], normals[indices[j+1]*3+2]);
                    glm::vec3 n3(normals[indices[j+2]*3], normals[indices[j+2]*3+1], normals[indices[j+2]*3+2]);
                    t.nor[0] = n1;
                    t.nor[1] = n2;
                    t.nor[2] = n3;
                }
                if(uvs.size() > 0)
                {
                    glm::vec3 t1(uvs[indices[j]*2], uvs[indices[j]*2+1], 0);
                    glm::vec3 t2(uvs[indices[j+1]*2], uvs[indices[j+1]*2+1], 0);
                    glm::vec3 t3(uvs[indices[j+2]*2], uvs[indices[j+2]*2+1], 0);
                    t.uv[0] = t1;
                    t.uv[1] = t2;
                    t.uv[2] = t3;
                }
                this->triangles.push_back(t);
            }
        }
//        std::cout << "" << std::endl;
        //TODO: .mtl file loading
    }
    else
    {
        //An error loading the OBJ occurred!
//        std::cout << errors << std::endl;
    }

    // Also write our triangles to texture
    int dummy;
    computeStorageDimensions(&triangleStorageSideLen, &dummy);
    trianglesAsTexture->create(false);
}
