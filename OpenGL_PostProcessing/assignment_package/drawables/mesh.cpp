#include "drawables.h"
#include "tinyobj/tiny_obj_loader.h"
#include <iostream>
#include "utils.h"

Mesh::Mesh(QOpenGLFunctions_3_3_Core *context, std::string objFilePath)
    : Drawable(context), objFilePath(objFilePath)
{}

Mesh::~Mesh()
{}

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
            std::vector<glm::vec3> glNor;
            std::vector<glm::vec2> glUV;

            for(unsigned int x = 0; x < positions.size(); x += 3)
            {
                glPos.push_back(glm::vec3(positions[x], positions[x + 1], positions[x + 2]));
                if(normalsExist)
                {
                    glNor.push_back(glm::vec3(normals[x], normals[x + 1], normals[x + 2]));
                }
            }

            if(uvsExist)
            {
                for(unsigned int x = 0; x < uvs.size(); x += 2)
                {
                    glUV.push_back(glm::vec2(uvs[x], uvs[x + 1]));
                }
            }

            glContext->glGenBuffers(1, &bufferIndex);
            glContext->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIndex);
            glContext->glBufferData(GL_ELEMENT_ARRAY_BUFFER, glIndices.size() * sizeof(GLuint), glIndices.data(), GL_STATIC_DRAW);

            glContext->glGenBuffers(1, &bufferPosition);
            glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferPosition);
            glContext->glBufferData(GL_ARRAY_BUFFER, glPos.size() * sizeof(glm::vec3), glPos.data(), GL_STATIC_DRAW);

            if(normalsExist)
            {
                glContext->glGenBuffers(1, &bufferNormal);
                glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferNormal);
                glContext->glBufferData(GL_ARRAY_BUFFER, glNor.size() * sizeof(glm::vec3), glNor.data(), GL_STATIC_DRAW);
            }

            if(uvsExist)
            {
                glContext->glGenBuffers(1, &bufferUV);
                glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferUV);
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
