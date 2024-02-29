#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include "drawables/drawables.h"

class ShaderProgram {
private:
    // See MyGL's `glContext` member for more info
    QOpenGLFunctions_3_3_Core *glContext;

    // These will serve as CPU-side handles to the components
    // that make up our shader program. They are given values
    // in createAndCompileShaderProgram(), which is called in
    // MyGL::initializeGL().
    GLuint vertShader;
    GLuint fragShader;
    GLuint shaderProgram;


    /// The following two maps store the CPU-side handles
    /// for the "in" and "uniform" variables written in
    /// the shader program used in this assignment.
    /// Each key is the literal name of the variable as it
    /// is written in the shader, e.g. vs_Pos or u_ScreenDimensions.
    /// Each value is the handle to the variable named by the key.
    std::unordered_map<std::string, GLint> shaderAttribVariableHandles;
    std::unordered_map<std::string, GLint> shaderUniformVariableHandles;
    void parseShaderSourceForVariables(char *vertSource, char *fragSource);


    // Prints any error messages from the shader program linking
    // process to the console.
    void printLinkInfoLog(int prog);
    // Prints any GLSL shader compiler errors to the console.
    void printShaderInfoLog(int shader);

public:
    ShaderProgram(QOpenGLFunctions_3_3_Core*);

    // This function is designed to initialize a vertex shader
    // object, a fragment shader object, and shader program object
    // on the GPU. It stores handles to these objects in the three
    // input GLuints. It also takes in a pair of file names to indicate
    // which .glsl files you want to serve as the source text for your
    // vertex shader and fragment shader.
    // This function reads these files and buffers their contents
    // to the GPU, where they are compiled. If any GLSL compilation
    // errors occur, they will be printed to the console by this
    // function.
    void createAndCompileShaderProgram(QString vertFile,
                                       QString fragFile);

    // Make all of the relevant OpenGL API calls
    // to draw the data stored in the vertex buffer objects
    // associated with the given Drawable.
    void draw(Drawable &d);

    // Calls glUseProgram in a public context
    void useProgram();

    // Used to add handles for `uniform` and `in` variables
    // for this shader. You should invoke these in
    // MyGL::getHandlesForShaderVariables()
    void addUniform(std::string name);
    void addAttrib(std::string name);

    // Used to access the handles for `uniform` and `in`
    // variables stored in the unordered_maps in this class.
    // Call these from MyGL whenever you need to get shader
    // variable handles.
    GLuint getUniformHandle(std::string name) const;
    GLuint getAttribHandle(std::string name) const;

    // Tells you if this shader program has a valid handle
    // for a given variable name.
    bool isUniformHandleValid(std::string name) const;
    bool isAttribHandleValid(std::string name) const;

};
