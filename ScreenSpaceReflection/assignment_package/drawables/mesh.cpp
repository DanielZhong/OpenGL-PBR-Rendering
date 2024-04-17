#include "drawables.h"
#include "tinyobj/tiny_obj_loader.h"
#include <iostream>
#include "utils.h"

Mesh::Mesh(QOpenGLFunctions_3_3_Core *context, std::string objFilePath)
    : Drawable(context), objFilePath(objFilePath)
{}

Mesh::~Mesh()
{}

inline void CoordinateSystem(const glm::vec3& v1, glm::vec3* v2, glm::vec3* v3)
{
    if (std::abs(v1.x) > std::abs(v1.y)) {
        *v2 = glm::vec3(-v1.z, 0, v1.x) / std::sqrt(v1.x * v1.x + v1.z * v1.z);
    }
    else {
        *v2 = glm::vec3(0, v1.z, -v1.y) / std::sqrt(v1.y * v1.y + v1.z * v1.z);
    }
    *v3 = glm::cross(v1, *v2);
}


void Mesh::initializeAndBufferGeometryData() {
    std::vector<tinyobj::shape_t> shapes; std::vector<tinyobj::material_t> materials;
    std::string errors = tinyobj::LoadObj(shapes, materials, objFilePath.c_str());
    std::cout << errors << std::endl;
    if(errors.size() == 0)
    {
        indexBufferLength = 0;
        //Read the information from the vector of shape_ts
        for(unsigned int i = 0; i < shapes.size(); i++)
        {
            std::vector<float> &positions = shapes[i].mesh.positions;
            std::vector<float> &normals = shapes[i].mesh.normals;
            std::vector<float> &uvs = shapes[i].mesh.texcoords;
            std::vector<unsigned int> &indices = shapes[i].mesh.indices;

            bool normalsExist = normals.size() > 0;
            bool uvsExist = uvs.size() > 0;


            std::vector<GLuint> glIndices;
            for(unsigned int ui : indices)
            {
                glIndices.push_back(ui);
            }
            std::vector<glm::vec3> glPos;
            std::vector<glm::vec3> glNor, glTan, glBit;
            std::vector<glm::vec2> glUV;

            for(unsigned int x = 0; x < positions.size(); x += 3)
            {
                glPos.push_back(glm::vec3(positions[x], positions[x + 1], positions[x + 2]));
                if(normalsExist)
                {
                    glm::vec3 nor(normals[x], normals[x + 1], normals[x + 2]);
                    glm::vec3 tan, bit;
                    CoordinateSystem(nor, &tan, &bit);
                    glNor.push_back(nor);
                    glTan.push_back(tan);
                    glBit.push_back(bit);
                }
            }

            if(uvsExist)
            {
                for(unsigned int x = 0; x < uvs.size(); x += 2)
                {
                    glUV.push_back(glm::vec2(uvs[x], uvs[x + 1]));
                }
            }

            generateBuffer(BufferType::INDEX);
            bindBuffer(BufferType::INDEX);
            glContext->glBufferData(GL_ELEMENT_ARRAY_BUFFER, glIndices.size() * sizeof(GLuint), glIndices.data(), GL_STATIC_DRAW);

            generateBuffer(BufferType::POSITION);
            bindBuffer(BufferType::POSITION);
            glContext->glBufferData(GL_ARRAY_BUFFER, glPos.size() * sizeof(glm::vec3), glPos.data(), GL_STATIC_DRAW);

            if(normalsExist)
            {
                generateBuffer(BufferType::NORMAL);
                bindBuffer(BufferType::NORMAL);
                glContext->glBufferData(GL_ARRAY_BUFFER, glNor.size() * sizeof(glm::vec3), glNor.data(), GL_STATIC_DRAW);

                generateBuffer(BufferType::TANGENT);
                bindBuffer(BufferType::TANGENT);
                glContext->glBufferData(GL_ARRAY_BUFFER, glTan.size() * sizeof(glm::vec3), glTan.data(), GL_STATIC_DRAW);

                generateBuffer(BufferType::BITANGENT);
                bindBuffer(BufferType::BITANGENT);
                glContext->glBufferData(GL_ARRAY_BUFFER, glBit.size() * sizeof(glm::vec3), glBit.data(), GL_STATIC_DRAW);
            }

            if(uvsExist)
            {
                generateBuffer(BufferType::UV);
                bindBuffer(BufferType::UV);
                glContext->glBufferData(GL_ARRAY_BUFFER, glUV.size() * sizeof(glm::vec2), glUV.data(), GL_STATIC_DRAW);
            }

            indexBufferLength += indices.size();
        }
    }
    else {
        //An error loading the OBJ occurred!
        std::cout << errors << std::endl;
    }
}
