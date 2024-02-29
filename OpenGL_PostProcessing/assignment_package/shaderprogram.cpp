#include "shaderprogram.h"
#include "utils.h"
#include <iostream>

ShaderProgram::ShaderProgram(QOpenGLFunctions_3_3_Core *context)
    : glContext(context)
{}

void ShaderProgram::draw(Drawable &d) {
    useProgram();
    if(isAttribHandleValid("vs_Pos")) {
        d.bindBuffer(POSITION);
        glContext->glEnableVertexAttribArray(getAttribHandle("vs_Pos"));
        glContext->glVertexAttribPointer(getAttribHandle("vs_Pos"), 3, GL_FLOAT, false, 0, nullptr);
    }
    printGLErrorLog();
    if(isAttribHandleValid("vs_Nor")) {
        d.bindBuffer(NORMAL);
        glContext->glEnableVertexAttribArray(getAttribHandle("vs_Nor"));
        glContext->glVertexAttribPointer(getAttribHandle("vs_Nor"), 3, GL_FLOAT, false, 0, nullptr);
    }
    printGLErrorLog();
    if(isAttribHandleValid("vs_UV")) {
        d.bindBuffer(UV);
        glContext->glEnableVertexAttribArray(getAttribHandle("vs_UV"));
        glContext->glVertexAttribPointer(getAttribHandle("vs_UV"), 2, GL_FLOAT, false, 0, nullptr);
    }

    printGLErrorLog();
    d.bindBuffer(INDEX);
    glContext->glDrawElements(GL_TRIANGLES, d.getIndexBufferLength(), GL_UNSIGNED_INT, 0);

    printGLErrorLog();
    if(isAttribHandleValid("vs_Pos")) {
        glContext->glDisableVertexAttribArray(getAttribHandle("vs_Pos"));
    }
    if(isAttribHandleValid("vs_Nor")) {
        glContext->glDisableVertexAttribArray(getAttribHandle("vs_Nor"));
    }
    if(isAttribHandleValid("vs_UV")) {
        glContext->glDisableVertexAttribArray(getAttribHandle("vs_UV"));
    }
    printGLErrorLog();
}

// A helper function for createAndCompileShaderProgram.
// It reads the contents of a file into a char*.
char* textFileRead(const char* fileName) {
    char* text = nullptr;

    if (fileName != nullptr) {
        FILE *file = fopen(fileName, "rt");

        if (file != nullptr) {
            fseek(file, 0, SEEK_END);
            int count = ftell(file);
            rewind(file);

            if (count > 0) {
                text = (char*)malloc(sizeof(char) * (count + 1));
                count = fread(text, sizeof(char), count, file);
                text[count] = '\0';	//cap off the string with a terminal symbol
            }
            fclose(file);
        }
    }
    return text;
}

void ShaderProgram::createAndCompileShaderProgram(QString vertFile,
                                                  QString fragFile) {

    // Set up the direct filepath to the vertex and fragment
    // shader files.
    QString projectPath = getCurrentPath();
    projectPath.append("/glsl/");
    QString vertPath = projectPath + vertFile;
    QString fragPath = projectPath + fragFile;

    // Make the OpenGL function calls that create a
    // vertex shader object, fragment shader object,
    // and shader program object on the GPU. We store
    // CPU-side handles to these GPU-side objects in
    // the GLuints passed into this function.
    vertShader = glContext->glCreateShader(GL_VERTEX_SHADER);
    fragShader = glContext->glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glContext->glCreateProgram();

    // Parse the plain-text contents of the vertex
    // and fragment shader files, storing them in C-style
    // char* strings (how OpenGL expects strings to be formatted).
    char *vertexShaderSource = textFileRead(vertPath.toStdString().c_str());
    char *fragmentShaderSource = textFileRead(fragPath.toStdString().c_str());

    // Send the contents of the shader files to the GPU
    glContext->glShaderSource(vertShader, 1, &vertexShaderSource, 0);
    glContext->glShaderSource(fragShader, 1, &fragmentShaderSource, 0);


    // Tell OpenGL on the GPU to try compiling the shader code
    // we just sent.
    glContext->glCompileShader(vertShader);
    glContext->glCompileShader(fragShader);
    // Check if everything compiled OK. If not, print out
    // any errors sent to us from the GPU-side shader compiler.
    GLint compiled;
    glContext->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cout << "Errors from " << vertFile.toStdString() << ":" << std::endl;
        printShaderInfoLog(vertShader);
    }
    glContext->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cout << "Errors from " << fragFile.toStdString() << ":" << std::endl;
        printShaderInfoLog(fragShader);
    }

    // Tell shaderProgramHandle that it manages
    // these particular vertex and fragment shaders
    glContext->glAttachShader(shaderProgram, vertShader);
    glContext->glAttachShader(shaderProgram, fragShader);
    glContext->glLinkProgram(shaderProgram);

    // Check for linking success
    GLint linked;
    glContext->glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        printLinkInfoLog(shaderProgram);
    }

    parseShaderSourceForVariables(vertexShaderSource, fragmentShaderSource);
    // Manually de-allocate the heap memory used to store the
    // shader contents. We don't need it now that it's been sent
    // to the GPU.
    free((char*)vertexShaderSource);
    free((char*)fragmentShaderSource);

}

// This function examines each line of the vertex and fragment shader source
// code for lines that begin with "uniform" or "in", then creates a CPU-side
// handle for that uniform or attribute variable.
void ShaderProgram::parseShaderSourceForVariables(char *vertSource, char *fragSource) {
    QString vs(vertSource);
    QString fs(fragSource);

    // Parse the uniforms and ins of the vertex shader
    QStringList vs_list = vs.split("\n", Qt::SkipEmptyParts);
    for(QString &line : vs_list) {
        QStringList sub = line.split(" ", Qt::SkipEmptyParts);
        if(sub[0] == "uniform") {
            this->addUniform(sub[2].left(sub[2].indexOf(";")).toStdString().c_str());
        }
        if(sub[0] == "in") {
            this->addAttrib(sub[2].left(sub[2].indexOf(";")).toStdString().c_str());
        }
    }
    // Parse the uniforms of the fragment shader
    QStringList fs_list = fs.split("\n", Qt::SkipEmptyParts);
    for(QString &line : fs_list) {
        QStringList sub = line.split(" ", Qt::SkipEmptyParts);
        if(sub[0] == "uniform") {
            this->addUniform(sub[2].left(sub[2].indexOf(";")).toStdString().c_str());
        }
    }
}


void ShaderProgram::printLinkInfoLog(int prog)
{
    GLint linked;
    glContext->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return;
    }
    std::cerr << "GLSL LINK ERROR" << std::endl;

    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glContext->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glContext->glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        std::cerr << "InfoLog:" << std::endl << infoLog << std::endl;
        delete[] infoLog;
    }
    // Throwing here allows us to use the debugger to track down the error.
    throw;
}

void ShaderProgram::useProgram() {
    glContext->glUseProgram(shaderProgram);
}


void ShaderProgram::addUniform(std::string name) {
    shaderUniformVariableHandles[name] = glContext->glGetUniformLocation(shaderProgram, name.c_str());
}

void ShaderProgram::addAttrib(std::string name) {
    shaderAttribVariableHandles[name] = glContext->glGetAttribLocation(shaderProgram, name.c_str());
}

GLuint ShaderProgram::getUniformHandle(std::string name) const {
    try {
        return shaderUniformVariableHandles.at(name);
    } catch(std::exception &e) {
        std::cout << "Error: No uniform with name " << name << " found!" << std::endl;
        return -1;
    }
}
GLuint ShaderProgram::getAttribHandle(std::string name) const {
    try {
        return shaderAttribVariableHandles.at(name);
    } catch(std::exception &e) {
        std::cout << "Error: No attribute with name " << name << " found!" << std::endl;
        return -1;
    }
}

bool ShaderProgram::isUniformHandleValid(std::string name) const {
    try {
        return shaderUniformVariableHandles.at(name) != -1;
    } catch(std::exception &e) {
        return false;
    }
}

bool ShaderProgram::isAttribHandleValid(std::string name) const {
    try {
        return shaderAttribVariableHandles.at(name) != -1;
    } catch(std::exception &e) {
        return false;
    }
}

void ShaderProgram::printShaderInfoLog(int shader)
{
    GLint compiled;
    glContext->glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return;
    }
    std::cerr << "GLSL COMPILE ERROR" << std::endl;

    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glContext->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glContext->glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
        std::cerr << "InfoLog:" << std::endl << infoLog << std::endl;
        delete[] infoLog;
    }
    // Throwing here allows us to use the debugger to track down the error.
    throw;
}
