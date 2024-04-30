#pragma once
#include "glm_includes.h"
#include "scene/entity.h"

//A perspective projection camera
//Receives its eye position and reference point from the scene XML file
class Camera : public Entity {
private:
    float m_fovy;
    unsigned int m_width, m_height;  // Screen dimensions
    float m_near_clip;  // Near clip plane distance
    float m_far_clip;  // Far clip plane distance
    float m_aspect;    // Aspect ratio
    float azimuth = 0.0f; // Horizontal angle, initialized to 0
    float elevation = 0.0f; // Vertical angle, initialized to 0

public:
    Camera(glm::vec3 pos);
    Camera(unsigned int w, unsigned int h, glm::vec3 pos);
    Camera(const Camera &c);
    void setWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;

    glm::mat4 getViewProj() const;
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    void adjustOrientation(float mouseX, float mouseY);
};
