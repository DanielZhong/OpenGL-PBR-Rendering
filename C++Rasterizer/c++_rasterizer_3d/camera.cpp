#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
    : forward(0.0f, 0.0f, -1.0f, 0.0f),
    right(1.0f, 0.0f, 0.0f, 0.0f),
    up(0.0f, 1.0f, 0.0f, 0.0f),
    position(0.0f, 0.0f, 10.0f, 1.0f),
    fov(45.0f),
    nearClip(0.01f),
    farClip(100.0f),
    aspectRatio(1.0f) {}

glm::mat4 Camera::GetViewMatrix() const {
    glm::vec3 R = glm::vec3(right);
    glm::vec3 U = glm::vec3(up);
    glm::vec3 F = glm::vec3(forward);
    glm::vec3 eye = glm::vec3(position);

    glm::mat4 orientation = {
        glm::vec4(R.x, U.x, F.x, 0.0f),
        glm::vec4(R.y, U.y, F.y, 0.0f),
        glm::vec4(R.z, U.z, F.z, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    };

    glm::mat4 translation = {
        glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(-eye, 1.0f)
    };

    return orientation * translation;
}

glm::mat4 Camera::GetPerspectiveMatrix() const {
    return glm::mat4(glm::vec4(1.f / (glm::tan(fov / 2) * aspectRatio), 0.f, 0.f, 0.f),
                     glm::vec4(0.f, 1.f / (glm::tan(fov / 2) * aspectRatio), 0.f, 0.f),
                     glm::vec4(0.f, 0.f, farClip / (farClip - nearClip), 1.f),
                     glm::vec4(0.f, 0.f, - farClip * nearClip / (farClip - nearClip), 0.f));

}

void Camera::TranslateForward(float amount) {
    position += forward * amount;
}

void Camera::TranslateRight(float amount) {
    position += right * amount;
}

void Camera::TranslateUp(float amount) {
    position += up * amount;
}

void Camera::RotateAboutRight(float degrees) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), degrees, glm::vec3(right));
    forward = rotation * forward;
    up = rotation * up;
}

void Camera::RotateAboutUp(float degrees) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), degrees, glm::vec3(up));
    forward = rotation * forward;
    right = rotation * right;
}

void Camera::RotateAboutForward(float degrees) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), degrees, glm::vec3(forward));
    up = rotation * up;
    right = rotation * right;
}
