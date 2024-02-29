#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetPerspectiveMatrix() const;

    void TranslateForward(float amount);
    void TranslateRight(float amount);
    void TranslateUp(float amount);

    void RotateAboutRight(float degrees);
    void RotateAboutUp(float degrees);
    void RotateAboutForward(float degrees);

    glm::vec3 GetForward() { return { forward.x, forward.y, forward.z }; }

    glm::vec3 GetPosition() { return { position.x, position.y, position.z }; }

private:
    glm::vec4 forward;
    glm::vec4 right;
    glm::vec4 up;
    glm::vec4 position;

    float fov;
    float nearClip;
    float farClip;
    float aspectRatio;

    void UpdateCameraVectors();
};

#endif // CAMERA_H
