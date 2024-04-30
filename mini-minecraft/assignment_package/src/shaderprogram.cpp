#include "shaderprogram.h"
#include <QFile>
#include <QStringBuilder>
#include <iostream>
#include <exception>
#include <QDir>


ShaderProgram::ShaderProgram(OpenGLContext *context)
    : vertShader(), fragShader(), prog(),
      context(context), m_isReloading(true)
{}

void ShaderProgram::destroy() {
    context->glDeleteProgram(prog);
    context->glDeleteShader(vertShader);
    context->glDeleteShader(fragShader);
    m_attribs.clear();
    m_unifs.clear();
}

void ShaderProgram::create(const char *vertfile, const char *fragfile)
{
    // Allocate space on our GPU for a vertex shader and a fragment shader and a shader program to manage the two
    vertShader = context->glCreateShader(GL_VERTEX_SHADER);
    fragShader = context->glCreateShader(GL_FRAGMENT_SHADER);
    prog = context->glCreateProgram();
    // Get the body of text stored in our two .glsl files
    QString qVertSource = qTextFileRead(vertfile);
    QString qFragSource = qTextFileRead(fragfile);

    char* vertSource = new char[qVertSource.size()+1];
    strcpy(vertSource, qVertSource.toStdString().c_str());
    char* fragSource = new char[qFragSource.size()+1];
    strcpy(fragSource, qFragSource.toStdString().c_str());

    // Send the shader text to OpenGL and store it in the shaders specified by the handles vertShader and fragShader
    context->glShaderSource(vertShader, 1, (const char**)&vertSource, 0);
    context->glShaderSource(fragShader, 1, (const char**)&fragSource, 0);
    // Tell OpenGL to compile the shader text stored above
    context->glCompileShader(vertShader);
    context->glCompileShader(fragShader);
    // Check if everything compiled OK
    GLint compiled;
    context->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(vertShader);
    }
    context->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(fragShader);
    }

    // Tell prog that it manages these particular vertex and fragment shaders
    context->glAttachShader(prog, vertShader);
    context->glAttachShader(prog, fragShader);
    context->glLinkProgram(prog);

    // Check for linking success
    GLint linked;
    context->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        printLinkInfoLog(prog);
    }

    parseShaderSourceForVariables(vertSource, fragSource);
    delete[] vertSource;
    delete[] fragSource;

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

void ShaderProgram::useMe() {
    context->glUseProgram(prog);
}

void ShaderProgram::setUnifMat4(std::string name, const glm::mat4 &m) {
    useMe();
    try {
        int handle = m_unifs.at(name);
        if(handle != -1) {
            context->glUniformMatrix4fv(handle, 1, GL_FALSE, &m[0][0]);
        }
    }
    catch(std::out_of_range &e) {
        std::cout << "Error: could not find shader variable with name " << name << std::endl;
    }
}

void ShaderProgram::setUnifVec2(std::string name, const glm::vec2 &v) {
    useMe();
    try {
        int handle = m_unifs.at(name);
        if(handle != -1) {
            context->glUniform2fv(handle, 1, &v[0]);
        }
    }
    catch(std::out_of_range &e) {
        std::cout << "Error: could not find shader variable with name " << name << std::endl;
    }
}
void ShaderProgram::setUnifVec3(std::string name, const glm::vec3 &v) {
    useMe();
    try {
        int handle = m_unifs.at(name);
        if(handle != -1) {
            context->glUniform3fv(handle, 1, &v[0]);
        }
    }
    catch(std::out_of_range &e) {
        std::cout << "Error: could not find shader variable with name " << name << std::endl;
    }
}
void ShaderProgram::setUnifFloat(std::string name, float f) {
    useMe();
    try {
        int handle = m_unifs.at(name);
        if(handle != -1) {
            context->glUniform1f(handle, f);
        }
    }
    catch(std::out_of_range &e) {
        std::cout << "Error: could not find shader variable with name " << name << std::endl;
    }
}
void ShaderProgram::setUnifInt(std::string name, int i) {
    useMe();
    try {
        int handle = m_unifs.at(name);
        if(handle != -1) {
            context->glUniform1i(handle, i);
        }
    }
    catch(std::out_of_range &e) {
        std::cout << "Error: could not find shader variable with name " << name << std::endl;
    }
}
void ShaderProgram::setUnifArrayInt(std::string name, int offset, int i) {
    useMe();
    try {
        int handle = m_unifs.at(name);
        if(handle != -1) {
            context->glUniform1i(handle + offset, i);
        }
    }
    catch(std::out_of_range &e) {
        std::cout << "Error: could not find shader variable with name " << name << std::endl;
    }
}

void ShaderProgram::setModelMatrix(const glm::mat4 &model) {
    useMe();

    if (m_unifs["u_Model"] != -1) {
        // Pass a 4x4 matrix into a uniform variable in our shader
        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(m_unifs["u_Model"],
                                    // How many matrices to pass
                                    1,
                                    // Transpose the matrix? OpenGL uses column-major, so no.
                                    GL_FALSE,
                                    // Pointer to the first element of the matrix
                                    &model[0][0]);
    }

    if (m_unifs["u_ModelInvTr"] != -1) {
        glm::mat4 modelinvtr = glm::inverse(glm::transpose(model));
        // Pass a 4x4 matrix into a uniform variable in our shader
        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(m_unifs["u_ModelInvTr"],
                                    // How many matrices to pass
                                    1,
                                    // Transpose the matrix? OpenGL uses column-major, so no.
                                    GL_FALSE,
                                    // Pointer to the first element of the matrix
                                    &modelinvtr[0][0]);
    }
}

void ShaderProgram::setUnifBool(const std::string& name, bool value) {
    useMe(); // Activate the shader program before setting the uniform

    // Attempt to find the uniform location and set its value
    try {
        int handle = m_unifs.at(name); // Retrieve the location of the uniform variable by name
        if (handle != -1) {
            context->glUniform1i(handle, static_cast<int>(value)); // OpenGL uses int for bool (0 or 1)
        }
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: could not find shader variable with name '" << name << "'" << std::endl;
    }
}



//This function, as its name implies, uses the passed in GL widget
void ShaderProgram::draw(Drawable &d) {
    if(d.elemCount(INDEX_QUAD) < 0) {
        throw std::invalid_argument(
                    "Attempting to draw a Drawable that has not initialized its count variable! Remember to set it to the length of your index array in create()."
                    );
    }
    useMe();

    int handle;
    if ((handle = m_attribs["vs_Pos"]) != -1 && d.bindBuffer(POSITION3)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 4, GL_FLOAT, false, 0, nullptr);
    }

    if ((handle = m_attribs["vs_UV"]) != -1 && d.bindBuffer(UV3)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 2, GL_FLOAT, false, 0, nullptr);
    }

    if ((handle = m_attribs["vs_Nor"]) != -1 && d.bindBuffer(NORMAL3)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 4, GL_FLOAT, false, 0, nullptr);
    }

    if ((handle = m_attribs["vs_Col"]) != -1 && d.bindBuffer(COLOR3)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 4, GL_FLOAT, false, 0, nullptr);
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindBuffer(INDEX_QUAD);
    context->glDrawElements(d.drawMode(), d.elemCount(INDEX_QUAD), GL_UNSIGNED_INT, 0);

    if (m_attribs["vs_Pos"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Pos"]);
    if (m_attribs["vs_Nor"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Nor"]);
    if (m_attribs["vs_Col"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Col"]);

    context->printGLErrorLog();
}

void ShaderProgram::drawInterleaved(Drawable &d)
{
    useMe();

    setUnifBool("u_Transparent", false);

    if(d.elemCount(INDEX) < 0) {
        throw std::out_of_range("Attempting to draw a drawable with INDEX of " + std::to_string(d.elemCount(INDEX)) + "!");
    }

    int size = 3 * sizeof(glm::vec4) + 2 * sizeof(glm::vec2) + 2 * sizeof(glm::vec3);

    if (m_attribs["vs_Pos"] != -1 && d.bindBuffer(POSITION)) {
        context->glEnableVertexAttribArray(m_attribs["vs_Pos"]);
        context->glVertexAttribPointer(m_attribs["vs_Pos"], 4, GL_FLOAT, false, size, (void*)0);
    }

    if (m_attribs["vs_Nor"] != -1 && d.bindBuffer(NORMAL)) {
        context->glEnableVertexAttribArray(m_attribs["vs_Nor"]);
        context->glVertexAttribPointer(m_attribs["vs_Nor"], 4, GL_FLOAT, false, size, (void*)sizeof(glm::vec4));
    }

    if (m_attribs["vs_Col"] != -1 && d.bindBuffer(COLOR)) {
        context->glEnableVertexAttribArray(m_attribs["vs_Col"]);
        context->glVertexAttribPointer(m_attribs["vs_Col"], 4, GL_FLOAT, false, size, (void*)(2 * sizeof(glm::vec4)));
    }

    if (m_attribs["vs_UV"]  != -1 && d.bindBuffer(UV))  {
        context->glEnableVertexAttribArray(m_attribs["vs_UV"]);
        context->glVertexAttribPointer(m_attribs["vs_UV"], 2, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)));
    }

    if (m_attribs["vs_Animated"]  != -1 && d.bindBuffer(ANIMATED))  {
        context->glEnableVertexAttribArray(m_attribs["vs_Animated"]);
        context->glVertexAttribPointer(m_attribs["vs_Animated"], 2, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)+ sizeof(glm::vec2)));
    }

    if (m_attribs["vs_Tangent"]  != -1 && d.bindBuffer(TANGENT))  {
        context->glEnableVertexAttribArray(m_attribs["vs_Tangent"]);
        context->glVertexAttribPointer(m_attribs["vs_Tangent"], 3, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)+ 2 * sizeof(glm::vec2)));
    }

    if (m_attribs["vs_Bitangent"]  != -1 && d.bindBuffer(BITANGENT))  {
        context->glEnableVertexAttribArray(m_attribs["vs_Bitangent"]);
        context->glVertexAttribPointer(m_attribs["vs_Bitangent"], 3, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)+ 2 * sizeof(glm::vec2)+ sizeof(glm::vec3)));
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindBuffer(INDEX);
    context->glDrawElements(d.drawMode(), d.elemCount(INDEX), GL_UNSIGNED_INT, 0);

    if (m_attribs["vs_Pos"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Pos"]);
    if (m_attribs["vs_Nor"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Nor"]);
    if (m_attribs["vs_Col"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Col"]);
    if (m_attribs["vs_UV"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_UV"]);
    if (m_attribs["vs_Animated"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Animated"]);
    if (m_attribs["vs_Tangent"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Tangent"]);
    if (m_attribs["vs_Bitangent"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Bitangent"]);

    context->printGLErrorLog();
}

void ShaderProgram::drawTrans(Drawable &d)
{
    useMe();
    setUnifBool("u_Transparent", true);
    if(d.elemCount(INDEX_TRAN) < 0) {
        throw std::out_of_range("Attempting to draw a drawable with INDEX of " + std::to_string(d.elemCount(INDEX_TRAN)) + "!");
    }

    int size = 3 * sizeof(glm::vec4) + 2 * sizeof(glm::vec2) + 2 * sizeof(glm::vec3);

    if (m_attribs["vs_Pos"] != -1 && d.bindBuffer(POSITION2)) {
        context->glEnableVertexAttribArray(m_attribs["vs_Pos"]);
        context->glVertexAttribPointer(m_attribs["vs_Pos"], 4, GL_FLOAT, false, size, (void*)0);
    }

    if (m_attribs["vs_Nor"] != -1 && d.bindBuffer(NORMAL2)) {
        context->glEnableVertexAttribArray(m_attribs["vs_Nor"]);
        context->glVertexAttribPointer(m_attribs["vs_Nor"], 4, GL_FLOAT, false, size, (void*)sizeof(glm::vec4));
    }

    if (m_attribs["vs_Col"] != -1 && d.bindBuffer(COLOR2)) {
        context->glEnableVertexAttribArray(m_attribs["vs_Col"]);
        context->glVertexAttribPointer(m_attribs["vs_Col"], 4, GL_FLOAT, false, size, (void*)(2 * sizeof(glm::vec4)));
    }

    if (m_attribs["vs_UV"]  != -1 && d.bindBuffer(UV2))  {
        context->glEnableVertexAttribArray(m_attribs["vs_UV"]);
        context->glVertexAttribPointer(m_attribs["vs_UV"], 2, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)));
    }

    if (m_attribs["vs_Animated"]  != -1 && d.bindBuffer(ANIMATED2))  {
        context->glEnableVertexAttribArray(m_attribs["vs_Animated"]);
        context->glVertexAttribPointer(m_attribs["vs_Animated"], 2, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)+ sizeof(glm::vec2)));
    }

    if (m_attribs["vs_Tangent"]  != -1 && d.bindBuffer(TANGENT2))  {
        context->glEnableVertexAttribArray(m_attribs["vs_Tangent"]);
        context->glVertexAttribPointer(m_attribs["vs_Tangent"], 3, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)+ 2 * sizeof(glm::vec2)));
    }

    if (m_attribs["vs_Bitangent"]  != -1 && d.bindBuffer(BITANGENT2))  {
        context->glEnableVertexAttribArray(m_attribs["vs_Bitangent"]);
        context->glVertexAttribPointer(m_attribs["vs_Bitangent"], 3, GL_FLOAT, false, size, (void*)(3 * sizeof(glm::vec4)+ 2 * sizeof(glm::vec2)+ sizeof(glm::vec3)));
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindBuffer(INDEX_TRAN);
    context->glDrawElements(d.drawMode(), d.elemCount(INDEX_TRAN), GL_UNSIGNED_INT, 0);

    if (m_attribs["vs_Pos"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Pos"]);
    if (m_attribs["vs_Nor"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Nor"]);
    if (m_attribs["vs_Col"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Col"]);
    if (m_attribs["vs_UV"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_UV"]);
    if (m_attribs["vs_Animated"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Animated"]);
    if (m_attribs["vs_Tangent"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Tangent"]);
    if (m_attribs["vs_Bitangent"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Bitangent"]);


    context->printGLErrorLog();
}

void ShaderProgram::drawInstanced(InstancedDrawable &d) {
    if(d.elemCount(INDEX) < 0) {
        throw std::invalid_argument(
                    "Attempting to draw a Drawable that has not initialized its count variable! Remember to set it to the length of your index array in create()."
                    );
    }
    useMe();

    int handle;
    if ((handle = m_attribs["vs_Pos"]) != -1 && d.bindBuffer(POSITION)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 4, GL_FLOAT, false, 0, nullptr);
        context->glVertexAttribDivisor(handle, 0);
    }

    if ((handle = m_attribs["vs_Nor"]) != -1 && d.bindBuffer(NORMAL)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 4, GL_FLOAT, false, 0, nullptr);
        context->glVertexAttribDivisor(handle, 0);
    }

    if ((handle = m_attribs["vs_ColInstanced"]) != -1 && d.bindBuffer(COLOR)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 3, GL_FLOAT, false, 0, nullptr);
        context->glVertexAttribDivisor(handle, 1);
    }

    if ((handle = m_attribs["vs_OffsetInstanced"]) != -1 && d.bindBuffer(INSTANCED_OFFSET)) {
        context->glEnableVertexAttribArray(handle);
        context->glVertexAttribPointer(handle, 3, GL_FLOAT, false, 0, nullptr);
        context->glVertexAttribDivisor(handle, 1);
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindBuffer(INDEX);
    context->glDrawElementsInstanced(d.drawMode(), d.elemCount(INDEX), GL_UNSIGNED_INT, 0, d.instanceCount());

    if (m_attribs["vs_Pos"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Pos"]);
    if (m_attribs["vs_Nor"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_Nor"]);
    if (m_attribs["vs_ColInstanced"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_ColInstanced"]);
    if (m_attribs["vs_OffsetInstanced"] != -1) context->glDisableVertexAttribArray(m_attribs["vs_OffsetInstanced"]);

    context->printGLErrorLog();
}

char* ShaderProgram::textFileRead(const char* fileName) {
    char* text = nullptr;

    if (fileName != NULL) {
        FILE *file = fopen(fileName, "rt");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            int count = ftell(file);
            rewind(file);

            if (count > 0) {
                text = (char*)malloc(sizeof(char) * (count + 1));
                count = fread(text, sizeof(char), count, file);
                text[count] = '\0';	//cap off the string with a terminal symbol, fixed by Cory
            }
            fclose(file);
        }
    }
    return text;
}

QString ShaderProgram::qTextFileRead(const char *fileName)
{
    QString text;
    QFile file(fileName);
    if(file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }
    return text;
}

void ShaderProgram::printShaderInfoLog(int shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
        qDebug() << "ShaderInfoLog:" << "\n" << infoLog << "\n";
        delete [] infoLog;
    }

    // should additionally check for OpenGL errors here
}

void ShaderProgram::printLinkInfoLog(int prog)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        qDebug() << "LinkInfoLog:" << "\n" << infoLog << "\n";
        delete [] infoLog;
    }
}
