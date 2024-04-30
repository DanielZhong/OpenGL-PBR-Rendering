#pragma once

#include <QOpenGLWidget>
#include <QTimer>
#include <QOpenGLExtraFunctions>


class OpenGLContext
    : public QOpenGLWidget,
      public QOpenGLExtraFunctions
{

public:
    OpenGLContext(QWidget *parent);
    ~OpenGLContext();

    void debugContextVersion();
    void printGLErrorLog();
    void printLinkInfoLog(int prog);
    void printShaderInfoLog(int shader);
};
