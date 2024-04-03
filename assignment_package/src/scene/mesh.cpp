#include "mesh.h"
#include "tinyobj/tiny_obj_loader.h"
#include <iostream>

Mesh::Mesh(OpenGLContext *context)
    : Drawable(context)
{}

Triangle::Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
    : points{p1, p2, p3},
      planeNormal(glm::normalize(glm::cross(p2 - p1, p3 - p2))),
      normals{planeNormal, planeNormal, planeNormal},
      uvs{glm::vec2(), glm::vec2(), glm::vec2()}
{}

void Mesh::LoadOBJ(const QString &filename, const QString &local_path)
{
    QString filepath = local_path; filepath.append(filename);
    std::vector<tinyobj::shape_t> shapes; std::vector<tinyobj::material_t> materials;
    std::string errors = tinyobj::LoadObj(shapes, materials, filepath.toStdString().c_str());
    std::cout << errors << std::endl;
    if(errors.size() == 0)
    {
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

                Triangle t(p1, p2, p3);
                if(normals.size() > 0)
                {
                    glm::vec3 n1(normals[indices[j]*3], normals[indices[j]*3+1], normals[indices[j]*3+2]);
                    glm::vec3 n2(normals[indices[j+1]*3], normals[indices[j+1]*3+1], normals[indices[j+1]*3+2]);
                    glm::vec3 n3(normals[indices[j+2]*3], normals[indices[j+2]*3+1], normals[indices[j+2]*3+2]);
                    t.normals[0] = n1;
                    t.normals[1] = n2;
                    t.normals[2] = n3;
                }
                if(uvs.size() > 0)
                {
                    glm::vec2 t1(uvs[indices[j]*2], uvs[indices[j]*2+1]);
                    glm::vec2 t2(uvs[indices[j+1]*2], uvs[indices[j+1]*2+1]);
                    glm::vec2 t3(uvs[indices[j+2]*2], uvs[indices[j+2]*2+1]);
                    t.uvs[0] = t1;
                    t.uvs[1] = t2;
                    t.uvs[2] = t3;
                }
                this->faces.push_back(t);
            }
        }
        std::cout << "" << std::endl;
        //TODO: .mtl file loading
    }
    else
    {
        //An error loading the OBJ occurred!
        std::cout << errors << std::endl;
    }
}

void Mesh::destroy() {
    Drawable::destroy();
    faces.clear();
}

void Mesh::create(){
    //Count the number of vertices for each face so we can get a count for the entire mesh
        std::vector<glm::vec3> vert_pos;
        std::vector<glm::vec3> vert_nor, vert_tan, vert_bit;
        std::vector<glm::vec2> vert_uv;
        std::vector<GLuint> vert_idx;

        unsigned int index = 0;

        for(unsigned int i = 0; i < faces.size(); i++){
            Triangle &tri = faces[i];
            vert_pos.push_back(tri.points[0]); vert_nor.push_back(tri.normals[0]); vert_uv.push_back(tri.uvs[0]);
            vert_pos.push_back(tri.points[1]); vert_nor.push_back(tri.normals[1]); vert_uv.push_back(tri.uvs[1]);
            vert_pos.push_back(tri.points[2]); vert_nor.push_back(tri.normals[2]); vert_uv.push_back(tri.uvs[2]);
            vert_idx.push_back(index++);vert_idx.push_back(index++);vert_idx.push_back(index++);

            glm::vec3 deltaPos1 = tri.points[1] - tri.points[0];
            glm::vec3 deltaPos2 = tri.points[2] - tri.points[0];
            glm::vec2 deltaUV1 = tri.uvs[1] - tri.uvs[0];
            glm::vec2 deltaUV2 = tri.uvs[2] - tri.uvs[0];
//            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = glm::normalize(deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y);
            glm::vec3 bitangent = glm::normalize(deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x);
            vert_tan.push_back(tangent); vert_tan.push_back(tangent); vert_tan.push_back(tangent);
            vert_bit.push_back(bitangent); vert_bit.push_back(bitangent); vert_bit.push_back(bitangent);
        }

        count = vert_idx.size();

        generateBuffer(IDX);
        bindBuffer(IDX);
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, vert_idx.size() * sizeof(GLuint), vert_idx.data(), GL_STATIC_DRAW);

        generateBuffer(POS);
        bindBuffer(POS);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vert_pos.size() * sizeof(glm::vec3), vert_pos.data(), GL_STATIC_DRAW);

        generateBuffer(NOR);
        bindBuffer(NOR);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vert_nor.size() * sizeof(glm::vec3), vert_nor.data(), GL_STATIC_DRAW);

        generateBuffer(TAN);
        bindBuffer(TAN);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vert_tan.size() * sizeof(glm::vec3), vert_tan.data(), GL_STATIC_DRAW);

        generateBuffer(BIT);
        bindBuffer(BIT);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vert_bit.size() * sizeof(glm::vec3), vert_bit.data(), GL_STATIC_DRAW);

        generateBuffer(UV);
        bindBuffer(UV);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vert_uv.size() * sizeof(glm::vec2), vert_uv.data(), GL_STATIC_DRAW);
}
