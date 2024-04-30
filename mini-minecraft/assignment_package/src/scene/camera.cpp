#include "camera.h"
#include "glm_includes.h"

Camera::Camera(glm::vec3 pos)
    : Camera(400, 400, pos)
{}

Camera::Camera(unsigned int w, unsigned int h, glm::vec3 pos)
    : Entity(pos), m_fovy(45), m_width(w), m_height(h),
      m_near_clip(0.1f), m_far_clip(1000.f), m_aspect(w / static_cast<float>(h))
{}

Camera::Camera(const Camera &c)
    : Entity(c),
      m_fovy(c.m_fovy),
      m_width(c.m_width),
      m_height(c.m_height),
      m_near_clip(c.m_near_clip),
      m_far_clip(c.m_far_clip),
      m_aspect(c.m_aspect)
{}


void Camera::setWidthHeight(unsigned int w, unsigned int h) {
    m_width = w;
    m_height = h;
    m_aspect = w / static_cast<float>(h);
}


void Camera::tick(float dT, InputBundle &input) {
    // Do nothing
}

glm::mat4 Camera::getViewProj() const {
    return glm::perspective(glm::radians(m_fovy), m_aspect, m_near_clip, m_far_clip) * glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::vec3 Camera::getForward() const {
    return m_forward;
}

glm::vec3 Camera::getRight() const {
    return m_right;
}

void Camera::adjustOrientation(float mouseX, float mouseY) {
    // Adjust angles based on mouse input
    azimuth += mouseX;
    elevation += mouseY;

    // Clamp elevation to prevent flipping
    elevation = std::max(-89.0f, std::min(89.0f, elevation));

    // Convert spherical (polar) coordinates to Cartesian coordinates for the forward vector
    glm::vec3 forward;
    forward.x = cos(glm::radians(elevation)) * sin(glm::radians(azimuth));
    forward.y = sin(glm::radians(elevation));
    forward.z = cos(glm::radians(elevation)) * cos(glm::radians(azimuth));
    m_forward = glm::normalize(forward);

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    m_right = glm::normalize(glm::cross(m_forward, worldUp));
    m_up = glm::cross(m_right, m_forward);
}
