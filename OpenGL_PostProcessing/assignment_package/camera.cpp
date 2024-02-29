#include "camera.h"
#include <iostream>


Camera::Camera(unsigned int w, unsigned int h)
    : fovy(55), aspect(w / (float)h), nearClip(0.1), farClip(1000),
      eye(0,0,10), target(0,0,0),
      forward(0,0,-1), up(0,1,0), right(glm::cross(forward, up))
{}

void Camera::recomputeAspectRatio(unsigned int w, unsigned int h) {
    aspect = w / (float)h;
}


glm::mat4 Camera::getViewProj() {
    return getProj() * getView();
}

glm::mat4 Camera::getView() {
    return glm::lookAt(eye, target, up);
}

glm::mat4 Camera::getProj() {
    return glm::perspective(glm::radians(fovy), aspect, nearClip, farClip);
}


void Camera::RotateAboutGlobalUp(float deg) {
    float rad = glm::radians(deg);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rad, up);

    glm::vec3 eyeDir = eye - target;
    eyeDir = glm::vec3(rotationMatrix * glm::vec4(eyeDir, 1.0));
    eye = target + eyeDir;

    forward = glm::normalize(target - eye);
    right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    up = glm::cross(right, forward);
}

void Camera::RotateAboutLocalRight(float deg) {
    float rad = glm::radians(deg);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rad, right);

    glm::vec3 eyeDir = eye - target;
    eyeDir = glm::vec3(rotationMatrix * glm::vec4(eyeDir, 1.0));
    eye = target + eyeDir;

    forward = glm::normalize(target - eye);
    up = glm::normalize(glm::cross(right, forward));
}

void Camera::Zoom(float amt) {
    glm::vec3 zoomDirection = glm::normalize(eye - target);
    eye -= zoomDirection * amt;

    forward = glm::normalize(target - eye);
}

void Camera::PanAlongRight(float amt) {
    eye += right * amt;
    target += right * amt;
}

void Camera::PanAlongUp(float amt) {
    eye += up * amt;
    target += up * amt;
}
