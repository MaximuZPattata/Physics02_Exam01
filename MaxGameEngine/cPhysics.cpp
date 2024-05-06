#include "cPhysics.h"
#include "sPhysicsProperties.h"
#include "OpenGLCommon.h"
#include "cMesh.h"

#include <iostream>
#include <vector>
#include <limits.h>
#include <float.h>

cPhysics::cPhysics()
{
}

cPhysics::~cPhysics()
{
}

void cPhysics::setVAOManager(cVAOManager* pTheMeshManager)
{
	this->m_pMeshManager = pTheMeshManager;
	return;
}

glm::vec3 cPhysics::ClosestPtPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	glm::vec3 bc = c - b;

	float snom = glm::dot(p - a, ab), sdenom = glm::dot(p - b, a - b);

	float tnom = glm::dot(p - a, ac), tdenom = glm::dot(p - c, a - c);

	if (snom <= 0.0f && tnom <= 0.0f)
		return a;

	float unom = glm::dot(p - b, bc), udenom = glm::dot(p - c, b - c);

	if (sdenom <= 0.0f && unom <= 0.0f)
		return b;

	if (tdenom <= 0.0f && udenom <= 0.0f)
		return c;

	glm::vec3 n = glm::cross(b - a, c - a);

	float vc = glm::dot(n, glm::cross(a - p, b - p));

	if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f)
		return a + snom / (snom + sdenom) * ab;

	float va = glm::dot(n, glm::cross(b - p, c - p));

	if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f)
		return b + unom / (unom + udenom) * bc;

	float vb = glm::dot(n, glm::cross(c - p, a - p));

	if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f)
		return a + tnom / (tnom + tdenom) * ac;

	float u = va / (va + vb + vc);
	float v = vb / (va + vb + vc);
	float w = 1.0f - u - v;

	return u * a + v * b + w * c;
}

bool cPhysics::CheckForAABBTriangleCollision(std::vector<sAABBTriangle> triangleList, sPhysicsProperties* spherePhysicalProps)
{
	glm::vec3 closestPointToTriangle = glm::vec3(0.0f);
	float closestDistanceSoFar = FLT_MAX;

	for (unsigned int index = 0; index < triangleList.size(); index ++)
	{
		glm::vec3 vertex1 = triangleList[index].vertex1;
		glm::vec3 vertex2 = triangleList[index].vertex2;
		glm::vec3 vertex3 = triangleList[index].vertex3;

		//-------------Calculate closest point-----------------------------------

		glm::vec3 closestPointToTriangle = ClosestPtPointTriangle(spherePhysicalProps->position, vertex1, vertex2, vertex3);

		spherePhysicalProps->sphereProps->distanceToTriangle = glm::distance(closestPointToTriangle, spherePhysicalProps->position);

		if (spherePhysicalProps->sphereProps->distanceToTriangle < closestDistanceSoFar)
		{
			closestDistanceSoFar = spherePhysicalProps->sphereProps->distanceToTriangle;

			spherePhysicalProps->sphereProps->closestTriangleVertices[0] = vertex1;
			spherePhysicalProps->sphereProps->closestTriangleVertices[1] = vertex2;
			spherePhysicalProps->sphereProps->closestTriangleVertices[2] = vertex3;
		}

		if (spherePhysicalProps->sphereProps->distanceToTriangle < spherePhysicalProps->sphereProps->radius)
		{
			sCollisionEvent triangleCollision;

			glm::vec3 edgeA = vertex2 - vertex1;
			glm::vec3 edgeB = vertex3 - vertex1;

			triangleCollision.triangleNormal = glm::normalize(glm::cross(edgeA, edgeB));

			triangleCollision.distanceToTriangle = spherePhysicalProps->sphereProps->distanceToTriangle;

			triangleCollision.contactPoint = closestPointToTriangle;

			spherePhysicalProps->sphereProps->ListOfCollisionsThisFrame.push_back(triangleCollision);
		}
	}

	//-----------------Check for collision------------------------------------------

	if (!spherePhysicalProps->sphereProps->ListOfCollisionsThisFrame.empty())
		return true;

	return false;
}

// Basically Triangle Collision Detection(single/multiple)
bool cPhysics::CheckForPlaneCollision(sModelDrawInfo* drawInfo, cMesh* groundMesh, sPhysicsProperties* spherePhysicalProps)
{
	glm::vec3 closestPointToTriangle = glm::vec3(0.0f);
	float closestDistanceSoFar = FLT_MAX;

		for (unsigned int index = 0; index < drawInfo->numberOfIndices; index += 3)
		{
			//---------------Calculate vertex position-----------------------------

			glm::vec3 verts[3];

			unsigned int triangleIndex_0 = drawInfo->pIndices[index];
			unsigned int triangleIndex_1 = drawInfo->pIndices[index + 1];
			unsigned int triangleIndex_2 = drawInfo->pIndices[index + 2];

			verts[0].x = drawInfo->pVertices[triangleIndex_0].x;
			verts[0].y = drawInfo->pVertices[triangleIndex_0].y;
			verts[0].z = drawInfo->pVertices[triangleIndex_0].z;

			verts[1].x = drawInfo->pVertices[triangleIndex_1].x;
			verts[1].y = drawInfo->pVertices[triangleIndex_1].y;
			verts[1].z = drawInfo->pVertices[triangleIndex_1].z;

			verts[2].x = drawInfo->pVertices[triangleIndex_2].x;
			verts[2].y = drawInfo->pVertices[triangleIndex_2].y;
			verts[2].z = drawInfo->pVertices[triangleIndex_2].z;

			//----------------Convert to world space------------------------------

			glm::mat4 matModel = glm::mat4(1.0f);

			glm::mat4 matTranslate = glm::translate(glm::mat4(1.0f),
				glm::vec3(groundMesh->drawPosition.x,
					groundMesh->drawPosition.y,
					groundMesh->drawPosition.z));

			glm::mat4 matRotate = glm::mat4_cast(groundMesh->get_qOrientation());

			glm::mat4 matScale = glm::scale(glm::mat4(1.0f),
				glm::vec3(groundMesh->drawScale.x,
					groundMesh->drawScale.y,
					groundMesh->drawScale.z));

			matModel = matModel * matTranslate;

			matModel = matModel * matRotate;

			matModel = matModel * matScale;

			glm::vec4 vertsWorld[3];

			vertsWorld[0] = (matModel * glm::vec4(verts[0], 1.0f));
			vertsWorld[1] = (matModel * glm::vec4(verts[1], 1.0f));
			vertsWorld[2] = (matModel * glm::vec4(verts[2], 1.0f));

			//-------------Calculate closest point-----------------------------------

			glm::vec3 closestPointToTriangle = ClosestPtPointTriangle(spherePhysicalProps->position, vertsWorld[0], vertsWorld[1], vertsWorld[2]);

			spherePhysicalProps->sphereProps->distanceToTriangle = glm::distance(closestPointToTriangle, spherePhysicalProps->position);

			if (spherePhysicalProps->sphereProps->distanceToTriangle < closestDistanceSoFar)
			{
				closestDistanceSoFar = spherePhysicalProps->sphereProps->distanceToTriangle;

				spherePhysicalProps->sphereProps->closestTriangleVertices[0] = vertsWorld[0];
				spherePhysicalProps->sphereProps->closestTriangleVertices[1] = vertsWorld[1];
				spherePhysicalProps->sphereProps->closestTriangleVertices[2] = vertsWorld[2];
			}

			if (spherePhysicalProps->sphereProps->distanceToTriangle < spherePhysicalProps->sphereProps->radius)
			{
				sCollisionEvent triangleCollision;

				glm::vec3 edgeA = vertsWorld[1] - vertsWorld[0];
				glm::vec3 edgeB = vertsWorld[2] - vertsWorld[0];

				triangleCollision.triangleNormal = glm::normalize(glm::cross(edgeA, edgeB));

				triangleCollision.distanceToTriangle = spherePhysicalProps->sphereProps->distanceToTriangle;

				triangleCollision.contactPoint = closestPointToTriangle;

				spherePhysicalProps->sphereProps->ListOfCollisionsThisFrame.push_back(triangleCollision);
			}
		}

		//-----------------Check for collision------------------------------------------

		if(!spherePhysicalProps->sphereProps->ListOfCollisionsThisFrame.empty())
			return true;

	return false;
}

bool cPhysics::CheckForSphereCollision(sPhysicsProperties* firstSphereProps, sPhysicsProperties* secondSphereProps)
{
	float distanceToCollision = glm::distance(firstSphereProps->position, secondSphereProps->position);

	float SumOfRadius = firstSphereProps->sphereProps->radius + secondSphereProps->sphereProps->radius;

	if (distanceToCollision <= SumOfRadius)
	{
		return true;
	}

	return false;
}

// Basically Triangle Collision Response(single/multiple)
void cPhysics::PlaneCollisionResponse(sPhysicsProperties* sphereMesh, double deltaTime)
{
	if (!sphereMesh->sphereProps->ListOfCollisionsThisFrame.empty())
	{
		glm::vec3 velocityBeforeCollision = sphereMesh->sphereProps->velocity;
		glm::vec3 sphereDirection = glm::normalize(velocityBeforeCollision);
		glm::vec3 reflectionVector = glm::vec3(0.0f);

		if (sphereMesh->sphereProps->ListOfCollisionsThisFrame.size() > 1) 
		{
			//-------------------------Response for multiple collisions-----------------------------------------------

			/*glm::vec3 OvervallImpulse = glm::vec3(0.0f);

			for (int collisionIndex = 0; collisionIndex < sphereMesh->sphereProps->ListOfCollisionsThisFrame.size(); collisionIndex++) 
			{
				reflectionVector = glm::reflect(sphereDirection, sphereMesh->sphereProps->ListOfCollisionsThisFrame[collisionIndex].triangleNormal);

				glm::vec3 closestPoint = sphereMesh->sphereProps->ListOfCollisionsThisFrame[collisionIndex].contactPoint;

				float relativeSpeed = glm::dot(velocityBeforeCollision, reflectionVector);

				if (relativeSpeed < 0) 
				{
					std::cout << "Calc Impulse" << std::endl;

					float coefficientOfRestitution = 1.0f;  // Determines kinetic energy retained or lost during collsion(Value reduces based on elasticity)
					//float impulse = -2.0f * relativeSpeed * coefficientOfRestitution; //Reduces impact

					float impulse = relativeSpeed * coefficientOfRestitution;
					OvervallImpulse += impulse * reflectionVector;
				}
			}
			 
			sphereMesh->sphereProps->velocity += OvervallImpulse * sphereMesh->sphereProps->inverse_mass;*/

			// NOTE for future self : If calculating with OverallImpulse, then comment the below for loop and calculate velocity accordingly
			for (int collisionIndex = 0; collisionIndex < sphereMesh->sphereProps->ListOfCollisionsThisFrame.size(); collisionIndex++)
				reflectionVector += glm::reflect(sphereDirection, sphereMesh->sphereProps->ListOfCollisionsThisFrame[collisionIndex].triangleNormal);

			glm::vec3 combinedReflectionVector = glm::normalize(reflectionVector);

			float sphereSpeed = glm::length(sphereMesh->sphereProps->velocity);

			glm::vec3 newVelocity = combinedReflectionVector * sphereSpeed;

			sphereMesh->sphereProps->velocity = newVelocity;

			std::cout << sphereMesh->modelName << " has hit multiple triangles this frame!" << std::endl;

			for (int collisionIndex = 0; collisionIndex < sphereMesh->sphereProps->ListOfCollisionsThisFrame.size(); collisionIndex++)
			{
				glm::vec3 closestPoint = sphereMesh->sphereProps->ListOfCollisionsThisFrame[collisionIndex].contactPoint;

				//-----------------------Checking multiple overlaps----------------------------------------------------

				float collisionOverlap = sphereMesh->sphereProps->radius - sphereMesh->sphereProps->ListOfCollisionsThisFrame[collisionIndex].distanceToTriangle;

				if (collisionOverlap > 0.0f)
				{
					std::cout << "MULTIPLE_TRIANGLE_OVERLAP : " << collisionOverlap << std::endl;

					glm::vec3 normals = glm::normalize(closestPoint - sphereMesh->position);

					sphereMesh->position -= collisionOverlap/2.0f * normals;
				}
			}
		}
		else
		{
			//--------------------------Response for single collision--------------------------------------------------

			int index = sphereMesh->sphereProps->ListOfCollisionsThisFrame.size() - 1;

			glm::vec3 triNormal = sphereMesh->sphereProps->ListOfCollisionsThisFrame[index].triangleNormal;

			reflectionVector = glm::reflect(sphereDirection, triNormal);

			float sphereSpeed = glm::length(sphereMesh->sphereProps->velocity);

			glm::vec3 newVelocity = reflectionVector * sphereSpeed;

			sphereMesh->sphereProps->velocity = newVelocity;

			//-------------------------------Check for overlaps---------------------------------------------------------

			glm::vec3 collisionPoint = sphereMesh->sphereProps->ListOfCollisionsThisFrame[index].contactPoint;

			glm::vec3 normals = glm::normalize(collisionPoint - sphereMesh->position);

			float collisionOverlap = sphereMesh->sphereProps->radius - sphereMesh->sphereProps->ListOfCollisionsThisFrame[index].distanceToTriangle;

			if (collisionOverlap > 0.0f)
			{
				std::cout << "SINGLE_TRIANGLE_OVERLAP : " << collisionOverlap << std::endl;
				sphereMesh->position -= collisionOverlap / 2.0f * normals;
			}
		}

		sphereMesh->sphereProps->ListOfCollisionsThisFrame.clear();
	}
}

void cPhysics::SphereCollisionResponse(sPhysicsProperties* firstSphereModel, sPhysicsProperties* secondSphereModel)
{
	glm::vec3 normals = glm::normalize(secondSphereModel->position - firstSphereModel->position); // Finding the perpendicular normal

	glm::vec3 relativeVelocity = secondSphereModel->sphereProps->velocity - firstSphereModel->sphereProps->velocity; // Difference in velocities between two spheres

	float relativeSpeed = glm::dot(relativeVelocity, normals); // Magnitude of relative velocity vector

	if (relativeSpeed < 0)
	{
		float impulse = (2 * relativeSpeed) / (firstSphereModel->sphereProps->inverse_mass + secondSphereModel->sphereProps->inverse_mass); // Change in momentum(vector quantity) 

		//impulse *= static_cast<float>(deltaTime);

		firstSphereModel->sphereProps->velocity += (impulse * (firstSphereModel->sphereProps->inverse_mass) * normals);
		secondSphereModel->sphereProps->velocity -= (impulse * (secondSphereModel->sphereProps->inverse_mass) * normals);

		//--------------------------------------Moving the spheres out of collision-------------------------------------------------------------

		// Overlap that has happened during collision
		float collisionOverlap = firstSphereModel->sphereProps->radius + secondSphereModel->sphereProps->radius - glm::distance(firstSphereModel->position, secondSphereModel->position);

		//overlap *= static_cast<float>(deltaTime);

		if (collisionOverlap > 0)
		{
			firstSphereModel->position -= collisionOverlap / 2.0f * normals;
			secondSphereModel->position += collisionOverlap / 2.0f * normals;
		}
	}
}

void cPhysics::EulerForwardIntegration(sPhysicsProperties* physicsModel, double deltaTime)
{
	//float dampingFactor = 1.0f; 

	glm::vec3 velocityChange = physicsModel->sphereProps->acceleration * static_cast<float>(deltaTime) * 1.5f;

	//physicsModel->sphereProps->velocity *= dampingFactor;

	physicsModel->sphereProps->velocity += velocityChange;

	glm::vec3 positionChange = physicsModel->sphereProps->velocity * static_cast<float>(deltaTime) * 1.5f;

	physicsModel->position.x += positionChange.x;
	physicsModel->position.y += positionChange.y;
	physicsModel->position.z += positionChange.z;
}

