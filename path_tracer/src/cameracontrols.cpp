#include "mygl.h"
#include <QKeyEvent>
#include <QApplication>

void MyGL::mousePressEvent(QMouseEvent *e)
{
    if(e->buttons() & (Qt::LeftButton | Qt::RightButton | Qt::MiddleButton))
    {
        m_mousePosPrev = glm::vec2(e->pos().x(), e->pos().y());
    }
    m_glCamera.RecomputeAttributes();
    m_progPathTracer.setUnifVec3("u_Eye", m_glCamera.eye);
    m_progPathTracer.setUnifVec3("u_Forward", m_glCamera.look);
    m_progPathTracer.setUnifVec3("u_Right", m_glCamera.right);
    m_progPathTracer.setUnifVec3("u_Up", m_glCamera.up);
    resetPathTracer();
    update();
}

void MyGL::mouseMoveEvent(QMouseEvent *e)
{
    glm::vec2 pos(e->pos().x(), e->pos().y());
    if(e->buttons() & Qt::LeftButton)
    {
        // Rotation
        glm::vec2 diff = 0.02f * (pos - m_mousePosPrev);
        m_mousePosPrev = pos;
        m_glCamera.RotatePhi(-diff.x);
        m_glCamera.RotateTheta(-diff.y);
    }
    else if(e->buttons() & Qt::RightButton)
    {
        glm::vec2 diff = 0.02f * (pos - m_mousePosPrev);
        m_mousePosPrev = pos;
        m_glCamera.Zoom(diff.y);
    }
    else if(e->buttons() & Qt::MiddleButton) {
        // Panning
        glm::vec2 diff = 0.05f * (pos - m_mousePosPrev);
        m_mousePosPrev = pos;
        m_glCamera.TranslateAlongRight(-diff.x);
        m_glCamera.TranslateAlongUp(diff.y);
    }
    m_glCamera.RecomputeAttributes();
    m_progPathTracer.setUnifVec3("u_Eye", m_glCamera.eye);
    m_progPathTracer.setUnifVec3("u_Forward", m_glCamera.look);
    m_progPathTracer.setUnifVec3("u_Right", m_glCamera.right);
    m_progPathTracer.setUnifVec3("u_Up", m_glCamera.up);
    resetPathTracer();
    update();
}

void MyGL::wheelEvent(QWheelEvent *e)
{
    m_glCamera.Zoom(e->angleDelta().y() * 0.02f);
    m_glCamera.RecomputeAttributes();
    m_progPathTracer.setUnifVec3("u_Eye", m_glCamera.eye);
    m_progPathTracer.setUnifVec3("u_Forward", m_glCamera.look);
    m_progPathTracer.setUnifVec3("u_Right", m_glCamera.right);
    m_progPathTracer.setUnifVec3("u_Up", m_glCamera.up);
    resetPathTracer();
    update();
}

void MyGL::keyPressEvent(QKeyEvent *e)
{

    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }

    switch(e->key())
    {
    case (Qt::Key_Escape):
        QApplication::quit();
        break;
    case (Qt::Key_Right):
        m_glCamera.RotateAboutUp(-amount);
        break;
    case (Qt::Key_Left):
        m_glCamera.RotateAboutUp(amount);
        break;
    case (Qt::Key_Up):
        m_glCamera.RotateAboutRight(-amount);
        break;
    case (Qt::Key_Down):
        m_glCamera.RotateAboutRight(amount);
        break;
    case (Qt::Key_1):
        m_glCamera.fovy += amount;
        break;
    case (Qt::Key_2):
        m_glCamera.fovy -= amount;
        break;
    case (Qt::Key_W):
        m_glCamera.TranslateAlongLook(amount);
        break;
    case (Qt::Key_S):
        m_glCamera.TranslateAlongLook(-amount);
        break;
    case (Qt::Key_D):
        m_glCamera.TranslateAlongRight(amount);
        break;
    case (Qt::Key_A):
        m_glCamera.TranslateAlongRight(-amount);
        break;
    case (Qt::Key_Q):
        m_glCamera.TranslateAlongUp(-amount);
        break;
    case (Qt::Key_E):
        m_glCamera.TranslateAlongUp(amount);
        break;
    case (Qt::Key_F):
        m_glCamera.Reset();
        break;
    }
    m_glCamera.RecomputeAttributes();
    m_progPathTracer.setUnifVec3("u_Eye", m_glCamera.eye);
    m_progPathTracer.setUnifVec3("u_Forward", m_glCamera.look);
    m_progPathTracer.setUnifVec3("u_Right", m_glCamera.right);
    m_progPathTracer.setUnifVec3("u_Up", m_glCamera.up);
    resetPathTracer();
    update();
}
