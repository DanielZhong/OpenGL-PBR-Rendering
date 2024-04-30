#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <openglcontext.h>
#include <glm_includes.h>
#include <glm/glm.hpp>
#include "drawable.h"
#include <unordered_map>

#define dict std::unordered_map


class ShaderProgram
{
public:
    GLuint vertShader; // A handle for the vertex shader stored in this shader program
    GLuint fragShader; // A handle for the fragment shader stored in this shader program
    GLuint prog;       // A handle for the linked shader program stored in this class

    dict<std::string, int> m_attribs;
    dict<std::string, int> m_unifs;

    bool m_isReloading;

public:
    ShaderProgram(OpenGLContext* context);
    // Sets up the requisite GL data and shaders from the given .glsl files
    void create(const char *vertfile, const char *fragfile);
    void destroy();
    inline void addAttrib(const char *name) {
        m_attribs[name] = context->glGetAttribLocation(prog, name);
    }
    inline void addUniform(const char *name) {
        m_unifs[name] = context->glGetUniformLocation(prog, name);
    }
    // Tells our OpenGL context to use this shader to draw things
    void useMe();

    void parseShaderSourceForVariables(char *vertSource, char *fragSource);

    void setUnifMat4(std::string name, const glm::mat4 &m);
    void setUnifVec2(std::string name, const glm::vec2 &v);
    void setUnifVec3(std::string name, const glm::vec3 &v);
    void setUnifFloat(std::string name, float f);
    void setUnifInt(std::string name, int i);
    void setUnifArrayInt(std::string name, int offset, int i);
    void setModelMatrix(const glm::mat4 &model);
    void setUnifTime(float time);

    // Draw the given object to our screen using this ShaderProgram's shaders
    void draw(Drawable &d);
    void drawInterleaved(Drawable &d);
    void drawTrans(Drawable &d);
    void drawInstanced(InstancedDrawable &d);
    // Utility function used in create()
    char* textFileRead(const char*);
    // Utility function that prints any shader compilation errors to the console
    void printShaderInfoLog(int shader);
    // Utility function that prints any shader linking errors to the console
    void printLinkInfoLog(int prog);

    QString qTextFileRead(const char*);
    void setUnifBool(const std::string& name, bool value);

private:
    OpenGLContext* context;   // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                            // we need to pass our OpenGL context to the Drawable in order to call GL functions
                            // from within this class.
};

#endif // SHADERPROGRAM_H
