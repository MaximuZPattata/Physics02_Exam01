#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <math.h>
#include <map>
#include <vector>
#include <string>

struct sAABBTriangle
{
    glm::vec3 vertex1;
    glm::vec3 vertex2;
    glm::vec3 vertex3;
};

class cAABB
{
public:
    std::string meshModelName = "";

    glm::vec3 minXYZ;
    glm::vec3 maxXYZ;

    bool objectInsideAABB = false;

    // Child AABBs
    std::map <unsigned int, cAABB*> childAABBList;

    std::vector <glm::vec3> verticesInsideAABBList;
    std::vector <sAABBTriangle> trianglesInsideAABBList;

    glm::vec3 getExtentsXYZ(void);

    bool IsInsideAABB(glm::vec3 vertex);

    static unsigned int static_getLocationIndex(glm::vec3 minXYZ, glm::vec3 extentXYZ)
    {
        unsigned int theX = (unsigned int)floor(minXYZ.x / extentXYZ.x);
        unsigned int theY = (unsigned int)floor(minXYZ.y / extentXYZ.y);
        unsigned int theZ = (unsigned int)floor(minXYZ.z / extentXYZ.z);

        unsigned int theIndex = (theX * 1000 * 1000) + (theY * 1000) + (theZ);

        if (minXYZ.x < 0)
            theIndex += 100'000'000;

        if (minXYZ.y < 0)
            theIndex += 100'000;

        if (minXYZ.z < 0)
            theIndex += 100;

        return theIndex;
    }
};


