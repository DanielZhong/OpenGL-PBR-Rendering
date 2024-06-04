#pragma once
#include "glm_includes.h"
#include "smartpointerhelp.h"
#include "shape.h"
#include <vector>

class BVHNode {
private:
    glm::vec3 minCorner, maxCorner;
    // When writing to texture, will need to know
    // these nodes' pixel coords
    uPtr<BVHNode> child_L, child_R;
    Triangle *tri;

public:
    BVHNode();
    ~BVHNode();
};

// Texture contents:
// 1 pixel for minCorner
// 1 pixel for maxCorner
// 1/3rd pixel for 1D index of child_L
// 1/3rd pixel for 1D index of child_R
// 1/3 pixel for index of triangle within tri texture storage
    // Actually pretty easy if a triangle knows its index in the mesh's
    // vector of Triangles, since it's just that * 9 (as a 1D index)
