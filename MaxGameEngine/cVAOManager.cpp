#include "cVAOManager.h"

#include "../OpenGLCommon.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <sstream>
#include <fstream>

void cVAOManager::setBasePath(std::string basePathWithoutSlash)
{
    this->m_basePathWithoutSlash = basePathWithoutSlash;
    return;
}

bool cVAOManager::LoadModelIntoVAO(std::string friendlyName, std::string fileName, sModelDrawInfo& drawInfo, unsigned int shaderProgramID, bool bIsDynamicBuffer)
{
    drawInfo.meshFileName = fileName;

    drawInfo.friendlyName = friendlyName;

    std::string fileAndPath = this->m_basePathWithoutSlash + "/" + fileName;

    if (!this->m_LoadTheFile(fileAndPath, drawInfo))
        return false;

    glGenVertexArrays(1, &(drawInfo.VAO_ID));

    glBindVertexArray(drawInfo.VAO_ID);

    glGenBuffers(1, &(drawInfo.VertexBufferID));

    glBindBuffer(GL_ARRAY_BUFFER, drawInfo.VertexBufferID);

    glBufferData(GL_ARRAY_BUFFER, sizeof(sVertex) * drawInfo.numberOfVertices,
        (GLvoid*)drawInfo.pVertices, (bIsDynamicBuffer ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));

    glGenBuffers(1, &(drawInfo.IndexBufferID));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawInfo.IndexBufferID);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        sizeof(unsigned int) * drawInfo.numberOfIndices,
        (GLvoid*)drawInfo.pIndices,
        GL_STATIC_DRAW);

    GLint vpos_location = glGetAttribLocation(shaderProgramID, "vPos");
    GLint vcol_location = glGetAttribLocation(shaderProgramID, "vCol");
    GLint vNormal_location = glGetAttribLocation(shaderProgramID, "vNormal");
    GLint vTextureCoords_location = glGetAttribLocation(shaderProgramID, "vTextureCoords");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 4, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, x));

    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 4, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, r));

    glEnableVertexAttribArray(vNormal_location);
    glVertexAttribPointer(vNormal_location, 4, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, nx));

    glEnableVertexAttribArray(vTextureCoords_location);
    glVertexAttribPointer(vTextureCoords_location, 2, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, u));

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(vpos_location);
    glDisableVertexAttribArray(vcol_location);
    glDisableVertexAttribArray(vNormal_location);
    glDisableVertexAttribArray(vTextureCoords_location);

    this->m_map_ModelName_to_VAOID[drawInfo.friendlyName] = drawInfo;

    return true;
}

bool cVAOManager::FindDrawInfoByModelName(std::string filename, sModelDrawInfo& drawInfo)
{
    std::map< std::string, sModelDrawInfo>::iterator itDrawInfo = this->m_map_ModelName_to_VAOID.find(filename);

    if (itDrawInfo == this->m_map_ModelName_to_VAOID.end())
        return false;

    drawInfo = itDrawInfo->second;
    return true;
}

//bool cVAOManager::m_LoadTheFile_Ply_XYZ_N_RGBA(std::string theFileName, sModelDrawInfo& drawInfo)
//{
//    std::ifstream theBunnyFile(theFileName.c_str());
//
//    if (!theBunnyFile.is_open())
//        return false;
//
//    std::string temp;
//
//    while (theBunnyFile >> temp)
//    {
//        if (temp == "vertex")
//            break;
//    };
//
//    theBunnyFile >> drawInfo.numberOfVertices;
//
//    while (theBunnyFile >> temp)
//    {
//        if (temp == "face")
//            break;
//    };
//
//    theBunnyFile >> drawInfo.numberOfTriangles;
//
//    drawInfo.numberOfIndices = drawInfo.numberOfTriangles * 3;
//
//    while (theBunnyFile >> temp)
//    {
//        if (temp == "end_header")
//            break;
//    };
//
//    struct sVertexPlyFile
//    {
//        float x;
//        float y;
//        float z;
//        float nx, ny, nz;
//        float r, g, b, a;
//    };
//
//    struct sTrianglePlyFile
//    {
//        unsigned int v0, v1, v2;
//    };
//
//    sVertexPlyFile* pTheVerticesFile = new sVertexPlyFile[drawInfo.numberOfVertices];
//
//    for (unsigned int index = 0; index != drawInfo.numberOfVertices; index++)
//    {
//        sVertexPlyFile tempVertex;
//        theBunnyFile >> tempVertex.x;
//        theBunnyFile >> tempVertex.y;
//        theBunnyFile >> tempVertex.z;
//
//        theBunnyFile >> tempVertex.nx;
//        theBunnyFile >> tempVertex.ny;
//        theBunnyFile >> tempVertex.nz;
//
//        theBunnyFile >> tempVertex.r;       tempVertex.r /= 255.0f;
//        theBunnyFile >> tempVertex.g;       tempVertex.g /= 255.0f;
//        theBunnyFile >> tempVertex.b;       tempVertex.b /= 255.0f;
//        theBunnyFile >> tempVertex.a;       tempVertex.a /= 255.0f;
//
//
//        pTheVerticesFile[index] = tempVertex;
//    }
//
//    sTrianglePlyFile* pTheTriangles = new sTrianglePlyFile[drawInfo.numberOfTriangles];
//
//    for (unsigned int index = 0; index != drawInfo.numberOfTriangles; index++)
//    {
//        sTrianglePlyFile tempTriangle;
//
//        unsigned int discard;
//        theBunnyFile >> discard;
//        theBunnyFile >> tempTriangle.v0;
//        theBunnyFile >> tempTriangle.v1;
//        theBunnyFile >> tempTriangle.v2;
//
//        pTheTriangles[index] = tempTriangle;
//    }
//
//    drawInfo.pVertices = new sVertex[drawInfo.numberOfVertices];
//    for (unsigned int vertIndex = 0; vertIndex != drawInfo.numberOfVertices; vertIndex++)
//    {
//        drawInfo.pVertices[vertIndex].x = pTheVerticesFile[vertIndex].x;
//        drawInfo.pVertices[vertIndex].y = pTheVerticesFile[vertIndex].y;
//        drawInfo.pVertices[vertIndex].z = pTheVerticesFile[vertIndex].z;
//        drawInfo.pVertices[vertIndex].w = 1.0f;
//
//
//        drawInfo.pVertices[vertIndex].nx = pTheVerticesFile[vertIndex].nx;
//        drawInfo.pVertices[vertIndex].ny = pTheVerticesFile[vertIndex].ny;
//        drawInfo.pVertices[vertIndex].nz = pTheVerticesFile[vertIndex].nz;
//        drawInfo.pVertices[vertIndex].nw = 1.0f;
//
//        drawInfo.pVertices[vertIndex].r = pTheVerticesFile[vertIndex].r;
//        drawInfo.pVertices[vertIndex].g = pTheVerticesFile[vertIndex].g;
//        drawInfo.pVertices[vertIndex].b = pTheVerticesFile[vertIndex].b;
//        drawInfo.pVertices[vertIndex].a = pTheVerticesFile[vertIndex].a;
//    }
//
//    drawInfo.pIndices = new unsigned int[drawInfo.numberOfIndices];
//
//    unsigned int elementIndex = 0;
//
//    for (unsigned int triIndex = 0; triIndex != drawInfo.numberOfTriangles; triIndex++)
//    {
//        drawInfo.pIndices[elementIndex + 0] = pTheTriangles[triIndex].v0;
//        drawInfo.pIndices[elementIndex + 1] = pTheTriangles[triIndex].v1;
//        drawInfo.pIndices[elementIndex + 2] = pTheTriangles[triIndex].v2;
//
//        elementIndex += 3;
//    }
//
//    return true;
//}

//bool cVAOManager::m_LoadTheFile_Ply_XYZ_N_RGBA_UV(std::string theFileName, sModelDrawInfo& drawInfo)
//{
//    const aiScene* scene = m_AssimpImporter.ReadFile(theFileName, aiProcess_GenNormals);
//
//    aiMesh* mesh = scene->mMeshes[0];
//
//    if (scene == 0 || !scene->HasMeshes())
//        return;
//
//   /* std::ifstream theBunnyFile(theFileName.c_str());
//
//    if (!theBunnyFile.is_open())
//        return false;*/
//
// /*   std::string temp;
//
//    while (theBunnyFile >> temp)
//    {
//        if (temp == "vertex")
//            break;
//    };
//
//    theBunnyFile >> drawInfo.numberOfVertices;
//
//    while (theBunnyFile >> temp)
//    {
//        if (temp == "face")
//            break;
//    };*/
//
//  /*  theBunnyFile >> drawInfo.numberOfTriangles;
//
//    drawInfo.numberOfIndices = drawInfo.numberOfTriangles * 3;
//
//    while (theBunnyFile >> temp)
//    {
//        if (temp == "end_header")
//            break;
//    };*/
//
//
//    unsigned int numVerticesInVertArray = mesh->mNumVertices;
//    unsigned int numIndicesInIndexArray = mesh->mNumFaces * 3;
//
//    drawInfo.numberOfVertices = numVerticesInVertArray;
//    drawInfo.numberOfIndices = numIndicesInIndexArray;
//
//    struct sVertexPlyFile
//    {
//        float x, y, z;
//        float nx, ny, nz;
//        float r, g, b, a;
//        float u, v;
//    };
//
//    struct sTrianglePlyFile
//    {
//        unsigned int v0, v1, v2;
//    };
//
//    sVertexPlyFile* pTheVerticesFile = new sVertexPlyFile[drawInfo.numberOfVertices];
//
//    for (unsigned int index = 0; index != drawInfo.numberOfVertices; index++)
//    {
//        sVertexPlyFile tempVertex;
//        theBunnyFile >> tempVertex.x;
//        theBunnyFile >> tempVertex.y;
//        theBunnyFile >> tempVertex.z;
//
//        theBunnyFile >> tempVertex.nx;
//        theBunnyFile >> tempVertex.ny;
//        theBunnyFile >> tempVertex.nz;
//
//        theBunnyFile >> tempVertex.r;       tempVertex.r /= 255.0f;
//        theBunnyFile >> tempVertex.g;       tempVertex.g /= 255.0f;
//        theBunnyFile >> tempVertex.b;       tempVertex.b /= 255.0f;
//        theBunnyFile >> tempVertex.a;       tempVertex.a /= 255.0f;
//
//        theBunnyFile >> tempVertex.u;
//        theBunnyFile >> tempVertex.v;
//
//        pTheVerticesFile[index] = tempVertex;
//    }
//
//    sTrianglePlyFile* pTheTriangles = new sTrianglePlyFile[drawInfo.numberOfTriangles];
//
//    for (unsigned int index = 0; index != drawInfo.numberOfTriangles; index++)
//    {
//        sTrianglePlyFile tempTriangle;
//
//        unsigned int discard;
//        theBunnyFile >> discard;
//        theBunnyFile >> tempTriangle.v0;
//        theBunnyFile >> tempTriangle.v1;
//        theBunnyFile >> tempTriangle.v2;
//
//        pTheTriangles[index] = tempTriangle;
//    }
//
//    drawInfo.pVertices = new sVertex[drawInfo.numberOfVertices];
//    for (unsigned int vertIndex = 0; vertIndex != drawInfo.numberOfVertices; vertIndex++)
//    {
//        drawInfo.pVertices[vertIndex].x = pTheVerticesFile[vertIndex].x;
//        drawInfo.pVertices[vertIndex].y = pTheVerticesFile[vertIndex].y;
//        drawInfo.pVertices[vertIndex].z = pTheVerticesFile[vertIndex].z;
//        drawInfo.pVertices[vertIndex].w = 1.0f;
//
//        drawInfo.pVertices[vertIndex].nx = pTheVerticesFile[vertIndex].nx;
//        drawInfo.pVertices[vertIndex].ny = pTheVerticesFile[vertIndex].ny;
//        drawInfo.pVertices[vertIndex].nz = pTheVerticesFile[vertIndex].nz;
//        drawInfo.pVertices[vertIndex].nw = 1.0f;
//
//        drawInfo.pVertices[vertIndex].r = pTheVerticesFile[vertIndex].r;
//        drawInfo.pVertices[vertIndex].g = pTheVerticesFile[vertIndex].g;
//        drawInfo.pVertices[vertIndex].b = pTheVerticesFile[vertIndex].b;
//        drawInfo.pVertices[vertIndex].a = pTheVerticesFile[vertIndex].a;
//
//        drawInfo.pVertices[vertIndex].u = pTheVerticesFile[vertIndex].u;
//        drawInfo.pVertices[vertIndex].v = pTheVerticesFile[vertIndex].v;
//    }
//
//    drawInfo.pIndices = new unsigned int[drawInfo.numberOfIndices];
//
//    unsigned int elementIndex = 0;
//
//    for (unsigned int triIndex = 0; triIndex != drawInfo.numberOfTriangles; triIndex++)
//    {
//        drawInfo.pIndices[elementIndex + 0] = pTheTriangles[triIndex].v0;
//        drawInfo.pIndices[elementIndex + 1] = pTheTriangles[triIndex].v1;
//        drawInfo.pIndices[elementIndex + 2] = pTheTriangles[triIndex].v2;
//
//        elementIndex += 3;
//    }
//
//    return true;
//}

bool cVAOManager::m_LoadTheFile(std::string fileName, sModelDrawInfo& drawInfo)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate);

    if (!scene)
        return false;

    const aiMesh* mesh = scene->mMeshes[0];

    drawInfo.meshFileName = fileName; 

    drawInfo.numberOfVertices = mesh->mNumVertices;
    drawInfo.pVertices = new sVertex[drawInfo.numberOfVertices];

    for (unsigned int i = 0; i < drawInfo.numberOfVertices; ++i)
    {
        const aiVector3D& vertex = mesh->mVertices[i];
        const aiVector3D& normal = mesh->mNormals[i];
        const aiColor4D& color = mesh->mColors[0][i]; 

        drawInfo.pVertices[i].x = vertex.x;
        drawInfo.pVertices[i].y = vertex.y;
        drawInfo.pVertices[i].z = vertex.z;

        if (mesh->HasVertexColors(0))
        {
            drawInfo.pVertices[i].r = color.r;
            drawInfo.pVertices[i].g = color.g;
            drawInfo.pVertices[i].b = color.b;
            drawInfo.pVertices[i].a = color.a;
        }

        if (mesh->HasNormals())
        {
            drawInfo.pVertices[i].nx = normal.x;
            drawInfo.pVertices[i].ny = normal.y;
            drawInfo.pVertices[i].nz = normal.z;
        }

        if (mesh->HasTextureCoords(0))
        {
            drawInfo.pVertices[i].u = mesh->mTextureCoords[0][i].x;
            drawInfo.pVertices[i].v = mesh->mTextureCoords[0][i].y;
        }

    }

    drawInfo.numberOfIndices = mesh->mNumFaces * 3;
    drawInfo.pIndices = new unsigned int[drawInfo.numberOfIndices];
    drawInfo.numberOfTriangles = mesh->mNumFaces;
    drawInfo.pTriangles = new sTriangle[drawInfo.numberOfTriangles];

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        drawInfo.pIndices[i * 3] = face.mIndices[0];
        drawInfo.pIndices[i * 3 + 1] = face.mIndices[1];
        drawInfo.pIndices[i * 3 + 2] = face.mIndices[2];

        drawInfo.pTriangles[i].v1.x = mesh->mVertices[face.mIndices[0]].x;
        drawInfo.pTriangles[i].v1.y = mesh->mVertices[face.mIndices[0]].y;
        drawInfo.pTriangles[i].v1.z = mesh->mVertices[face.mIndices[0]].z;

        drawInfo.pTriangles[i].v2.x = mesh->mVertices[face.mIndices[1]].x;
        drawInfo.pTriangles[i].v2.y = mesh->mVertices[face.mIndices[1]].y;
        drawInfo.pTriangles[i].v2.z = mesh->mVertices[face.mIndices[1]].z;

        drawInfo.pTriangles[i].v3.x = mesh->mVertices[face.mIndices[2]].x;
        drawInfo.pTriangles[i].v3.y = mesh->mVertices[face.mIndices[2]].y;
        drawInfo.pTriangles[i].v3.z = mesh->mVertices[face.mIndices[2]].z;
    }

    return true;
}

// Only updates the vertex buffer information
bool cVAOManager::UpdateVertexBuffers(std::string fileName, sModelDrawInfo& updatedDrawInfo, unsigned int shaderProgramID)
{
    sModelDrawInfo updatedDrawInfo_TEMP;
    
    if (!this->FindDrawInfoByModelName(fileName, updatedDrawInfo_TEMP))
        return false;

    glBindBuffer(GL_ARRAY_BUFFER, updatedDrawInfo.VertexBufferID);

    // Original call to create and copy the initial data:
    //glBufferData(GL_ARRAY_BUFFER, sizeof(sVertex) * updatedDrawInfo.numberOfVertices, (GLvoid*)updatedDrawInfo.pVertices, GL_DYNAMIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sVertex) * updatedDrawInfo.numberOfVertices, (GLvoid*)updatedDrawInfo.pVertices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool cVAOManager::UpdateVAOBuffers(std::string fileName, sModelDrawInfo& updatedDrawInfo, unsigned int shaderProgramID)
{
    sModelDrawInfo updatedDrawInfo_TEMP;

    if (!this->FindDrawInfoByModelName(fileName, updatedDrawInfo_TEMP))
        return false;

    glBindBuffer(GL_ARRAY_BUFFER, updatedDrawInfo.VertexBufferID);

    // Original call to create and copy the initial data:
    // glBufferData(GL_ARRAY_BUFFER, sizeof(sVertex) * updatedDrawInfo.numberOfVertices, (GLvoid*)updatedDrawInfo.pVertices, GL_DYNAMIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sVertex) * updatedDrawInfo.numberOfVertices, (GLvoid*)updatedDrawInfo.pVertices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, updatedDrawInfo.IndexBufferID);

    // Original call to create and copy the initial data:
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * updatedDrawInfo.numberOfIndices, (GLvoid*)updatedDrawInfo.pIndices, GL_DYNAMIC_DRAW);

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned int) * updatedDrawInfo.numberOfIndices, (GLvoid*)updatedDrawInfo.pIndices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}


