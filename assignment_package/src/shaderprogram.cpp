#include "shaderprogram.h"
#include <QFile>
#include <QStringBuilder>
#include <iostream>
#include <exception>


ShaderProgram::ShaderProgram(OpenGLContext *context)
    : vertShader(), fragShader(), prog(),
      attrPos(-1), attrNor(-1), attrTan(-1), attrBit(-1), attrUV(-1),
      unifModel(-1), unifModelInvTr(-1), unifViewProj(-1), unifCamPos(-1),
      unifAlbedo(-1), unifMetallic(-1), unifRoughness(-1), unifAO(-1),
      context(context)
{}

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
    context->glShaderSource(vertShader, 1, &vertSource, 0);
    context->glShaderSource(fragShader, 1, &fragSource, 0);
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

    // Get the handles to the variables stored in our shaders
    // See shaderprogram.h for more information about these variables

    attrPos = context->glGetAttribLocation(prog, "vs_Pos");
    attrNor = context->glGetAttribLocation(prog, "vs_Nor");
    attrTan = context->glGetAttribLocation(prog, "vs_Tan");
    attrBit = context->glGetAttribLocation(prog, "vs_Bit");
    attrUV = context->glGetAttribLocation(prog, "vs_UV");
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
//This function, as its name implies, uses the passed in GL widget
void ShaderProgram::draw(Drawable &d)
{
    if(d.elemCount() < 0) {
        throw std::invalid_argument(
        "Attempting to draw a Drawable that has not initialized its count variable! Remember to set it to the length of your index array in create()."
        );
    }
    useMe();

    if (attrPos != -1 && d.bindBuffer(POS)) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 3, GL_FLOAT, false, 0, nullptr);
    }

    if (attrNor != -1 && d.bindBuffer(NOR)) {
        context->glEnableVertexAttribArray(attrNor);
        context->glVertexAttribPointer(attrNor, 3, GL_FLOAT, false, 0, nullptr);
    }

    if (attrTan != -1 && d.bindBuffer(TAN)) {
        context->glEnableVertexAttribArray(attrTan);
        context->glVertexAttribPointer(attrTan, 3, GL_FLOAT, false, 0, nullptr);
    }

    if (attrBit != -1 && d.bindBuffer(BIT)) {
        context->glEnableVertexAttribArray(attrBit);
        context->glVertexAttribPointer(attrBit, 3, GL_FLOAT, false, 0, nullptr);
    }

    if (attrUV != -1 && d.bindBuffer(UV)) {
        context->glEnableVertexAttribArray(attrUV);
        context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false, 0, nullptr);
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindBuffer(IDX);
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrTan != -1) context->glDisableVertexAttribArray(attrTan);
    if (attrBit != -1) context->glDisableVertexAttribArray(attrBit);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);

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
