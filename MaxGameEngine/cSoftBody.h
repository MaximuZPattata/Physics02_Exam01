#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "sModelDrawInfo.h"
#include "cMesh.h"

class cSoftBody // Verlet
{
public:
	cSoftBody();
	~cSoftBody();

	struct sNode
	{
		glm::vec3 currentPosition = glm::vec3(0.0f);
		glm::vec3 oldPosition = glm::vec3(0.0f);

		sVertex* modelVertexPointer = NULL;
	};

	struct sConstraint
	{
		sNode* nodeA = NULL;
		sNode* nodeB = NULL;

		float restLength = 0.0f;

		unsigned int iterationsCount = 1;

		bool bIsActive = true;
	};

	bool bActivateTearDown = false;
	bool bLockFlagPos = false;

	// Just gonna add a bunch of small spheres moving through the flag to make it look like the wind did !
	struct sGustOfWind
	{
		float radius = 0.0f;
		glm::vec3 windPos = glm::vec3(0.0f);
		glm::vec3 destinationPos = glm::vec3(0.0f);
		glm::vec3 acceleration = glm::vec3(0.0f);

		float timeElapsed = 0.f;
	};

	glm::vec3 acceleration = glm::vec3(0.0f);

	std::vector < sNode* > nodesList;
	std::vector < sConstraint* > constraintsList;
	std::vector < std::string > debugSpheresMeshNameList;
	std::vector < sGustOfWind* > sphereWindList;

	sModelDrawInfo mModelVertexInfo;

	const double MAX_DELTATIME = 1.0 / 60.0;

	int WIND_SPHERE_COUNT = 7;

	bool bEnableDebugSphere = false;
	bool bNodesShared = false;

	bool CreateSoftBody(sModelDrawInfo& modelDrawInfo, glm::mat4 matInitalTransform = glm::mat4(1.0f));

	// Will update the vertex information to match the particles
	void UpdateVertexPositions();

	void UpdateNormals(void);

	void CreateRandomBracing(unsigned int numberOfBraces, float minDistanceBetweenVertices);

	void VerletUpdate(double deltaTime);

	void ApplyCollision(double deltaTime);

	void SatisfyConstraints(void);

	void CleanZeros(glm::vec3& value);

	float CalculateDistance(sNode* nodeA, sNode* nodeB);
};

