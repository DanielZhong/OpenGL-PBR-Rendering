#pragma once

#include "utils.h"

//A perspective projection camera
class Camera
{
public:
    float fovy;
    float aspect;
    float nearClip;  // Near clip plane distance
    float farClip;  // Far clip plane distance

    //Computed attributes

    glm::vec3 eye,      //The position of the camera in world space
              target,      //The point in world space towards which the camera is pointing
              forward,     //The normalized vector from eye to ref. Is also known as the camera's "forward" vector.
              up,       //The normalized vector pointing upwards IN CAMERA SPACE. This vector is perpendicular to LOOK and RIGHT.
              right;    //The normalized vector pointing rightwards IN CAMERA SPACE. It is perpendicular to UP and LOOK.

public:
    Camera(unsigned int w, unsigned int h);

    void recomputeAspectRatio(unsigned int w, unsigned int h);

    glm::mat4 getViewProj();
    glm::mat4 getView();
    glm::mat4 getProj();

    void RotateAboutGlobalUp(float deg);
    void RotateAboutLocalRight(float deg);

    void PanAlongRight(float amt);
    void PanAlongUp(float amt);

    void Zoom(float amt);
};
