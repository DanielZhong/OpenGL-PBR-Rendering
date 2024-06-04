#include "bvhtree.h"

BVHNode::BVHNode()
    : minCorner(), maxCorner(),
      child_L(nullptr), child_R(nullptr),
      tri(nullptr)
{}

BVHNode::~BVHNode() {}

