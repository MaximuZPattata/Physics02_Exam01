#include "cSoftBody.h"

#if _DEBUG 
#include <iostream>
#endif

cSoftBody::cSoftBody() {}

cSoftBody::~cSoftBody() {}

bool cSoftBody::CreateSoftBody(sModelDrawInfo& modelInfo, glm::mat4 matInitalTransform)
{
	this->mModelVertexInfo = modelInfo;

	//--------Copying Vertices to a local copy ----------------------

	sVertex* localVertices = new sVertex[modelInfo.numberOfVertices];

	for (unsigned int index = 0; index != modelInfo.numberOfVertices; index++)
	{
		glm::vec4 theVertex = glm::vec4(modelInfo.pVertices[index].x, modelInfo.pVertices[index].y, modelInfo.pVertices[index].z, 1.0f);

		theVertex = matInitalTransform * theVertex;

		localVertices[index].x = theVertex.x;
		localVertices[index].y = theVertex.y;
		localVertices[index].z = theVertex.z;

		localVertices[index].w = modelInfo.pVertices[index].w;

		localVertices[index].r = modelInfo.pVertices[index].r;
		localVertices[index].g = modelInfo.pVertices[index].g;
		localVertices[index].b = modelInfo.pVertices[index].b;
		localVertices[index].a = modelInfo.pVertices[index].a;

		localVertices[index].nx = modelInfo.pVertices[index].nx;
		localVertices[index].ny = modelInfo.pVertices[index].ny;
		localVertices[index].nz = modelInfo.pVertices[index].nz;
		localVertices[index].nw = modelInfo.pVertices[index].nw;

		localVertices[index].u = modelInfo.pVertices[index].u;
		localVertices[index].v = modelInfo.pVertices[index].v;
	}

	//----------------------Copying Indices to a local copy--------------------------------------

	unsigned int* localIndices = new unsigned int[modelInfo.numberOfIndices];

	for (unsigned int index = 0; index != modelInfo.numberOfIndices; index++)
		localIndices[index] = modelInfo.pIndices[index];

	//---------------Pointing the model draw pointers to the local-------------------------------

	this->mModelVertexInfo.pVertices = localVertices;
	this->mModelVertexInfo.pIndices = localIndices;

	//------------------------Pushing to the node list-------------------------------------------

	this->nodesList.reserve(this->mModelVertexInfo.numberOfVertices);

	for (unsigned int index = 0; index != this->mModelVertexInfo.numberOfVertices; index++)
	{
		sNode* particleNode = new sNode();

		particleNode->currentPosition.x = this->mModelVertexInfo.pVertices[index].x;
		particleNode->currentPosition.y = this->mModelVertexInfo.pVertices[index].y;
		particleNode->currentPosition.z = this->mModelVertexInfo.pVertices[index].z;

		this->mModelVertexInfo.pVertices[index] = localVertices[index];

		particleNode->oldPosition = particleNode->currentPosition;

		particleNode->modelVertexPointer = &(this->mModelVertexInfo.pVertices[index]);

		this->nodesList.push_back(particleNode);
	}

	//--------------------------Creating Contraints----------------------------------------------

	for (unsigned int index = 0; index < this->mModelVertexInfo.numberOfIndices; index += 3)
	{
		sNode* node1 = this->nodesList[this->mModelVertexInfo.pIndices[index]];
		sNode* node2 = this->nodesList[this->mModelVertexInfo.pIndices[index + 1]];
		sNode* node3 = this->nodesList[this->mModelVertexInfo.pIndices[index + 2]];

		sConstraint* edge1 = new sConstraint();
		edge1->nodeA = node1;
		edge1->nodeB = node2;
		edge1->restLength = this->CalculateDistance(edge1->nodeA, edge1->nodeB);

		sConstraint* edge2 = new sConstraint();
		edge2->nodeA = node2;
		edge2->nodeB = node3;
		edge2->restLength = this->CalculateDistance(edge2->nodeA, edge2->nodeB);

		sConstraint* edge3 = new sConstraint();
		edge3->nodeA = node3;
		edge3->nodeB = node1;
		edge3->restLength = this->CalculateDistance(edge3->nodeA, edge3->nodeB);

		this->constraintsList.push_back(edge1);
		this->constraintsList.push_back(edge2);
		this->constraintsList.push_back(edge3);
	}

	return true;
}

void cSoftBody::UpdateVertexPositions()
{


	for (sNode* currentNode : this->nodesList)
	{
		currentNode->modelVertexPointer->x = currentNode->currentPosition.x;
		currentNode->modelVertexPointer->y = currentNode->currentPosition.y;
		currentNode->modelVertexPointer->z = currentNode->currentPosition.z;
	}
}

void cSoftBody::UpdateNormals(void)
{
	for (unsigned int vertIndex = 0; vertIndex != this->mModelVertexInfo.numberOfVertices; vertIndex++)
	{
		this->mModelVertexInfo.pVertices[vertIndex].nx = 0.0f;
		this->mModelVertexInfo.pVertices[vertIndex].ny = 0.0f;
		this->mModelVertexInfo.pVertices[vertIndex].nz = 0.0f;
	}

	for (unsigned int triIndex = 0; triIndex != this->mModelVertexInfo.numberOfTriangles; triIndex++)
	{
		unsigned int vertAIndex = this->mModelVertexInfo.pIndices[triIndex + 0];
		unsigned int vertBIndex = this->mModelVertexInfo.pIndices[triIndex + 1];
		unsigned int vertCIndex = this->mModelVertexInfo.pIndices[triIndex + 2];

		sVertex& vertexA = this->mModelVertexInfo.pVertices[vertAIndex];
		sVertex& vertexB = this->mModelVertexInfo.pVertices[vertBIndex];
		sVertex& vertexC = this->mModelVertexInfo.pVertices[vertCIndex];

		glm::vec3 vertA = glm::vec3(vertexA.x, vertexA.y, vertexA.z);
		glm::vec3 vertB = glm::vec3(vertexB.x, vertexB.y, vertexB.z);
		glm::vec3 vertC = glm::vec3(vertexC.x, vertexC.y, vertexC.z);

		glm::vec3 triangleEdgeAtoB = vertB - vertA;
		glm::vec3 triangleEdgeAtoC = vertC - vertA;

		glm::vec3 theNormal = glm::cross(triangleEdgeAtoB, triangleEdgeAtoC);

		theNormal = glm::normalize(theNormal);

		vertexA.nx += theNormal.x;
		vertexA.ny += theNormal.y;
		vertexA.nz += theNormal.z;

		vertexB.nx += theNormal.x;
		vertexB.ny += theNormal.y;
		vertexB.nz += theNormal.z;

		vertexC.nx += theNormal.x;
		vertexC.ny += theNormal.y;
		vertexC.nz += theNormal.z;
	}

	for (unsigned int vertIndex = 0; vertIndex != this->mModelVertexInfo.numberOfVertices; vertIndex++)
	{
		glm::vec3 newNormal = glm::vec3(this->mModelVertexInfo.pVertices[vertIndex].nx, this->mModelVertexInfo.pVertices[vertIndex].ny, this->mModelVertexInfo.pVertices[vertIndex].nz);

		newNormal = glm::normalize(newNormal);

		this->mModelVertexInfo.pVertices[vertIndex].nx = newNormal.x;
		this->mModelVertexInfo.pVertices[vertIndex].ny = newNormal.y;
		this->mModelVertexInfo.pVertices[vertIndex].nz = newNormal.z;
	}

	return;
}

float cSoftBody::CalculateDistance(sNode* pPartA, sNode* pPartB)
{
	return glm::distance(pPartA->currentPosition, pPartB->currentPosition);
}

void cSoftBody::VerletUpdate(double deltaTime)
{
	if (deltaTime > MAX_DELTATIME)
		deltaTime = MAX_DELTATIME;

	for (sNode* currentNode : nodesList)
	{
		glm::vec3 currentPos = currentNode->currentPosition;
		glm::vec3 oldPos = currentNode->oldPosition;

		currentNode->currentPosition += (currentPos - oldPos) + (this->acceleration * static_cast<float>(deltaTime * deltaTime));

		currentNode->oldPosition = currentPos;

		this->CleanZeros(currentNode->currentPosition);
		this->CleanZeros(currentNode->oldPosition);
	}
}

void cSoftBody::ApplyCollision(double deltaTime)
{
	/*if (!bNodesShared)
	{
		for (int i = 0; i < nodesList.size(); i++)
		{
			if(nodesList[i]->currentPosition.x < -7.0f)
				std::cout << "Node index : " << i << " | pos : (" << nodesList[i]->currentPosition.x << ", " << nodesList[i]->currentPosition.y << ", " << nodesList[i]->currentPosition.z << ")" << std::endl;
		}

		bNodesShared = true;
	}*/

	//-------------------------------Passing spheres as winds------------------------------------

	for (unsigned int nodeCount = 0; nodeCount < nodesList.size(); nodeCount++)
	{
		if (nodeCount != 2'499 && nodeCount != 1'147 && nodeCount != 47)
		{
			for (sGustOfWind* currentWindSphere : sphereWindList)
			{
				currentWindSphere->acceleration = (currentWindSphere->destinationPos - currentWindSphere->windPos) * 0.05f;

				currentWindSphere->windPos += 0.5f * (currentWindSphere->acceleration * static_cast<float>(deltaTime * deltaTime));

				float distanceToSphere = glm::distance(nodesList[nodeCount]->currentPosition, currentWindSphere->windPos);

				currentWindSphere->timeElapsed += static_cast<float>(deltaTime) * static_cast<float>(deltaTime);

				if (distanceToSphere < currentWindSphere->radius)
				{
					glm::vec3 particleToCentreRay = nodesList[nodeCount]->currentPosition - currentWindSphere->windPos;

					particleToCentreRay = glm::normalize(particleToCentreRay);

					nodesList[nodeCount]->currentPosition = (particleToCentreRay * currentWindSphere->radius) + currentWindSphere->windPos;
				}

				// Deleting the spheres after a few seconds
				if (currentWindSphere->timeElapsed > 100.0f)
					sphereWindList.erase(std::remove(sphereWindList.begin(), sphereWindList.end(), currentWindSphere), sphereWindList.end());
			}
		}
	}

	//--------------------------Nodes holding the flag to the pole-------------------------------
	
	if (bLockFlagPos)
	{
		this->nodesList[2'499]->currentPosition = glm::vec3(-7.0f, 6.5f, 0.0f);
		//this->nodesList[1'147]->currentPosition = glm::vec3(-7.0f, 0.f, 0.0f);
		this->nodesList[47]->currentPosition = glm::vec3(-7.0f, -7.5f, 0.0f);
	}
}

void cSoftBody::SatisfyConstraints(void)
{
	const unsigned int NUM_ITERATIONS = 5;

	for (unsigned int iteration = 0; iteration != NUM_ITERATIONS; iteration++)
	{
		for (sConstraint* currentConstraint : this->constraintsList)
		{
			if (currentConstraint->bIsActive)
			{
				cSoftBody::sNode* pX1 = currentConstraint->nodeA;
				cSoftBody::sNode* pX2 = currentConstraint->nodeB;

				glm::vec3 deltaPos = pX2->currentPosition - pX1->currentPosition;

				float deltaLength = glm::length(deltaPos);

				float diff = (deltaLength - currentConstraint->restLength) / deltaLength;

				if (bActivateTearDown && diff > 0.9f )
					currentConstraint->bIsActive = false;

				// Bouncy < 1.0 to 2.0 < Breaks down
				float tightnessFactor = 1.f;

				pX1->currentPosition += deltaPos * 0.5f * diff * tightnessFactor;
				pX2->currentPosition -= deltaPos * 0.5f * diff * tightnessFactor;

				this->CleanZeros(pX1->currentPosition);
				this->CleanZeros(pX2->currentPosition);
			}
		}
	}
}

void cSoftBody::CleanZeros(glm::vec3& value)
{
	const float minFloat = 1.192092896e-07f;

	if ((value.x < minFloat) && (value.x > -minFloat))
	{
		value.x = 0.0f;
	}
	if ((value.y < minFloat) && (value.y > -minFloat))
	{
		value.y = 0.0f;
	}
	if ((value.z < minFloat) && (value.z > -minFloat))
	{
		value.z = 0.0f;
	}
}

void cSoftBody::CreateRandomBracing(unsigned int numberOfBraces, float minDistanceBetweenVertices)
{
	for (unsigned int count = 0; count != numberOfBraces; count++)
	{
		bool bKeepLookingForParticles = false;

		do
		{
			bKeepLookingForParticles = false;

			unsigned int particleIndex1 = rand() % this->nodesList.size();
			unsigned int particleIndex2 = rand() % this->nodesList.size();

			sNode* node1 = this->nodesList[particleIndex1];
			sNode* node2 = this->nodesList[particleIndex2];

			float distBetween = this->CalculateDistance(node1, node2);

			if (distBetween < minDistanceBetweenVertices)
				bKeepLookingForParticles = true;
			else
			{
				sConstraint* bracingConstraint = new sConstraint();

				bracingConstraint->nodeA = node1;
				bracingConstraint->nodeB = node2;
				bracingConstraint->restLength = this->CalculateDistance(bracingConstraint->nodeA, bracingConstraint->nodeB);

				this->constraintsList.push_back(bracingConstraint);
			}
		} while (bKeepLookingForParticles);
	}

	return;
}
