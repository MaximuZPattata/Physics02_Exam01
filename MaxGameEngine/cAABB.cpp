#include "cAABB.h"

glm::vec3 cAABB::getExtentsXYZ(void)
{
    return maxXYZ - minXYZ;
}

bool cAABB::IsInsideAABB(glm::vec3 vertex)
{
    if (vertex.x < minXYZ.x || vertex.x > maxXYZ.x || vertex.y < minXYZ.y || vertex.y > maxXYZ.y || vertex.z < minXYZ.z || vertex.z > maxXYZ.z)
        return false;

    return true;
}
