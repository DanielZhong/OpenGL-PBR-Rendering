#include "player.h"
#include <QString>

#define WALK_SPEED 5.f
#define JUMP_SPEED 10.f
#define MOUSE_SENSITY 0.05f
#define BLOCK_DISTANCE 3.f
#define BLOCK_TYPE DIRT

const static std::array<glm::vec3, 8> playerCorners = {
    glm::vec3(-0.5f, 0, -0.5f), // Bottom Back Left
    glm::vec3(0.5f, 0, -0.5f),  // Bottom Back Right
    glm::vec3(-0.5f, 0, 0.5f),  // Bottom Front Left
    glm::vec3(0.5f, 0, 0.5f),    // Bottom Front
    glm::vec3(-0.5f, 1, -0.5f), // Top Back Left
    glm::vec3(0.5f, 1, -0.5f),  // Top Back Right
    glm::vec3(-0.5f, 1, 0.5f),  // Top Front Left
    glm::vec3(0.5f, 1, 0.5f)    // Top Front Right
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    Ray(glm::vec3 o, glm::vec3 d)
        : origin(o), direction(d) {}
};

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

bool Player::OnGrounded(const Terrain &terrain) {
    bool isOn = false;
    // Calculate the world position of the corner slightly below the player
    glm::vec3 worldPos = this->m_position - glm::vec3(0.5f, 0, 0.5f); // Slightly below to ensure contact with the terrain
    for (int x = 0; x <= 1; x++) {
        for (int z = 0; z <= 1; z++) {
            // Convert the world position to grid coordinates
            glm::vec3 gridPos = glm::vec3(floor(worldPos.x) + x, floor(worldPos.y - 0.005f), floor(worldPos.z) + z);
            BlockType cell = terrain.getGlobalBlockAt(gridPos);
            if (cell != EMPTY && cell != WATER && cell != LAVA) {
                isOn = true;
            } else {
                isOn = false;
            }
        }
    }
    return isOn;
}

bool Player::InLavaWater(const Terrain &terrain, BlockType type) {
    bool isIn = false;
    // Calculate the world position of the corner slightly below the player
    glm::vec3 worldPos = this->m_position + glm::vec3(0.5f, 1.5f, 0.5f); // Slightly below to ensure contact with the terrain
    for (int x = 0; x <= 1; x++) {
        for (int z = 0; z <= 1; z++) {
            // Convert the world position to grid coordinates
            glm::vec3 gridPos = glm::vec3(floor(worldPos.x) + x, floor(worldPos.y - 0.005f), floor(worldPos.z) + z);
            BlockType cell = terrain.getGlobalBlockAt(gridPos);
            if (cell == type) {
                isIn = true;
            }
        }
    }
    return isIn;
}

// cited from lecture slides
bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getGlobalBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY && cellType != WATER && cellType != LAVA) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

bool gridMarch_PaceBlock(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = BLOCK_DISTANCE; // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        BlockType cellType = terrain.getGlobalBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY && cellType != WATER && cellType != LAVA) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

void Player::processInputs(InputBundle &inputs) {
    float speedMultiplier = inputs.shiftPressed ? 2.0f : 1.0f;
    float decelerationFactor = 0.95f;
    m_camera.adjustOrientation(inputs.mouseX * MOUSE_SENSITY, inputs.mouseY * MOUSE_SENSITY);

    glm::vec3 movementDirection = glm::vec3(0);
    glm::vec3 cameraForward = glm::normalize(m_camera.getForward());
    glm::vec3 cameraRight = glm::normalize(m_camera.getRight());

    // Calculate horizontal movement direction based on input keys
    if (inputs.wPressed) movementDirection += cameraForward;
    if (inputs.sPressed) movementDirection -= cameraForward;
    if (inputs.dPressed) movementDirection += cameraRight;
    if (inputs.aPressed) movementDirection -= cameraRight;

    bool isMoving = glm::length(movementDirection) > 0;

    // Normalize the horizontal movement direction if moving
    if (isMoving) {
         movementDirection = glm::normalize(movementDirection);
         movementDirection.y = 0;
      // Ensure no vertical component is included
    }

    // Handle movement in flight mode
    if (flightMode) {
        const float maxVerticalSpeed = 3.0f;

        // Adjust vertical velocity for downward movement, ensuring it doesn't exceed the maximum speed
        if (inputs.qPressed) {
            m_velocity.y = std::max(m_velocity.y - 0.4f, -maxVerticalSpeed);
        }else if (!inputs.ePressed) { // If neither 'q' nor 'e' is pressed, reset vertical velocity to zero
            if (m_velocity.y < 0) {
                m_velocity.y = 0;
            }
        }

        // Adjust vertical velocity for upward movement, ensuring it doesn't exceed the maximum speed
        if (inputs.ePressed || inputs.spacePressed) {
            m_velocity.y = std::min(m_velocity.y + 0.4f, maxVerticalSpeed);
        }

        // Apply increased speed for horizontal movement in flight mode
        m_velocity.x = movementDirection.x * WALK_SPEED * 2;
        m_velocity.z = movementDirection.z * WALK_SPEED * 2;
    }

    // Apply horizontal movement
    if (isMoving) {
        if(InLavaWater(mcr_terrain, WATER) || InLavaWater(mcr_terrain, LAVA)) {
            glm::vec3 horizontalVelocity = movementDirection * WALK_SPEED * 0.6f * speedMultiplier;
            m_velocity.x = horizontalVelocity.x;
            m_velocity.z = horizontalVelocity.z;
            if(!flightMode)
                m_velocity.y = -1;
            else {
                m_velocity.y = 0;
            }
        }
        else {
            glm::vec3 horizontalVelocity = movementDirection * WALK_SPEED * speedMultiplier;
            m_velocity.x = horizontalVelocity.x;
            m_velocity.z = horizontalVelocity.z;
        }
    }

    // Handle jump in non-flight mode
    if (!flightMode && inputs.spacePressed && OnGrounded(mcr_terrain)) {
        m_velocity.y = JUMP_SPEED; // Apply jump velocity if on the ground and spacebar is pressed
    }

    if (!flightMode && inputs.spacePressed && (InLavaWater(mcr_terrain, WATER) || InLavaWater(mcr_terrain, LAVA))) {
        m_velocity.y = 0.5 * JUMP_SPEED; // Apply jump velocity if on the ground and spacebar is pressed
    }

    // Gradual deceleration if no input, not affecting vertical movement during jump or flight ascent
    if (!isMoving) {
        m_velocity.x *= decelerationFactor;
        m_velocity.z *= decelerationFactor;
    }

    if((InLavaWater(mcr_terrain, WATER) || InLavaWater(mcr_terrain, LAVA)) && !inputs.spacePressed && !flightMode) {
        m_velocity.y = -2.f;
    }

    // Apply gravity in non-flight mode when not on the ground
    if (!flightMode && !OnGrounded(mcr_terrain)) {
        m_acceleration.y = -9.81f;
    } else if (!flightMode) {
        // Reset acceleration if on the ground in non-flight mode
        m_acceleration.y = 0;
    }

    // Ensure that vertical deceleration does not occur during a jump or upward movement in flight mode
    if (!inputs.spacePressed && !inputs.ePressed && m_velocity.y > 0) {
        m_velocity.y *= decelerationFactor; // Apply deceleration to y-axis only when not actively jumping or moving upward in flight mode
    }

    // Reset acceleration if no movement direction and velocity is negligible, to avoid drifting
    if (!isMoving && glm::length(m_acceleration) < 0.001f) {
        m_acceleration = glm::vec3(0);
    }
}



void Player::computePhysics(float dT, const Terrain &terrain) {
    if(flightMode) {
        m_velocity += m_acceleration * dT;
        this->moveAlongVector(m_velocity * dT);
        m_acceleration = glm::vec3(0.f);
        return;
    }
    bool isOnGround = OnGrounded(terrain);

    if (!isOnGround) {
        m_acceleration.y = -9.81f;
    } else {
        m_acceleration.y = 0;
    }

    m_velocity += m_acceleration * dT;

    glm::vec3 displacement = m_velocity * dT;

    glm::vec3 adjustedDisplacement(0); // Initialize with no displacement

    // Check for collisions along each axis independently
    for (int axis = 0; axis < 3; ++axis) {
        float axisMovement = displacement[axis];
        if (axisMovement == 0) continue; // Skip if no movement along this axis

        float shortestDistance = FLT_MAX; // Start with the maximum possible distance

        // Test collision for each corner of the player's bounding box
        for (const glm::vec3 &corner : playerCorners) {
            glm::vec3 worldCorner = m_position + corner; // World position of the corner
            glm::vec3 direction(0); // Initialize direction with zero vector
            direction[axis] = axisMovement; // Set movement along the current axis
            Ray ray(worldCorner, direction);

            glm::ivec3 blockHit;
            float dist;
            // Perform grid march to find potential collisions along the ray
            if (gridMarch(ray.origin, ray.direction, terrain, &dist, &blockHit)) {
                // Adjust shortestDistance if a closer collision is detected
                if (dist < shortestDistance) {
                    shortestDistance = dist;
                }
            }
        }

        // If a collision was detected, adjust the displacement
        if (shortestDistance != FLT_MAX) {
            if (!isOnGround && axis == 1 && m_velocity.y < 0) {
                adjustedDisplacement.y = (shortestDistance - 0.001f) * glm::sign(axisMovement); // Adjust based on player's height and a small offset to avoid floating point errors
            } else {
                adjustedDisplacement[axis] = (shortestDistance - 0.001f) * glm::sign(axisMovement); // Subtract a small value to prevent floating point errors
                m_velocity[axis] = 0; // Stop movement along this axis due to collision
            }

        } else {
            // No collision detected, proceed with the original displacement
            adjustedDisplacement[axis] = axisMovement;
        }
    }

    // Move the player by the adjusted displacement
    moveAlongVector(adjustedDisplacement);
}

void Player::removeBlock(Terrain &terrain) {
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = m_camera.getForward() * BLOCK_DISTANCE;
    glm::ivec3 blockHit;
    float dist;

    bool cameraHit =gridMarch( rayOrigin, rayDirection, terrain, &dist, &blockHit);

    if (cameraHit) {
        terrain.blockInteraction(blockHit.x, blockHit.y, blockHit.z, EMPTY);
    }
}

void Player::placeBlock(Terrain &terrain) {
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = m_camera.getForward();
    glm::ivec3 blockHit;
    float dist;

    bool newBlockHit = gridMarch_PaceBlock(rayOrigin, rayDirection, terrain, &dist, &blockHit);

    if (newBlockHit && dist <= BLOCK_DISTANCE) {
        glm::vec3 toOriginDir = glm::normalize(rayOrigin - (glm::vec3(blockHit) + 0.5f)); // Adding 0.5 to center the blockHit position
        // Determine the face of the blockHit that was impacted
        glm::ivec3 faceDir = glm::ivec3(round(toOriginDir.x), round(toOriginDir.y), round(toOriginDir.z));

        // Calculate the placement position adjacent to the hit block, in the direction the player is facing
        glm::ivec3 placementPos = blockHit + faceDir;

        // Ensure the placement position is empty before placing a new block
        if (terrain.getGlobalBlockAt(placementPos.x, placementPos.y, placementPos.z) == EMPTY) {
            terrain.blockInteraction(placementPos.x, placementPos.y, placementPos.z, BLOCK_TYPE);
        }
    }
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
