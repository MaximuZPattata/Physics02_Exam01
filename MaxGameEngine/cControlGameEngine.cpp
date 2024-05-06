#include "cControlGameEngine.h"
#include "OpenGLCommon.h"
#include "cVAOManager.h"
#include "cShaderManager.h"
#include "GLWF_CallBacks.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <string>

//-------------------------------------------------Private Functions-----------------------------------------------------------------------

cMesh* cControlGameEngine::g_pFindMeshByFriendlyName(std::string friendlyNameToFind)
{
    for (unsigned int index = 0; index != TotalMeshList.size(); index++)
    {
        if (TotalMeshList[index]->friendlyName == friendlyNameToFind)
            return TotalMeshList[index];
    }

    std::cout << "Cannot find mesh model for the name provided : " << friendlyNameToFind << std::endl;

    return NULL;
}

sModelDrawInfo* cControlGameEngine::g_pFindModelInfoByFriendlyName(std::string friendlyNameToFind)
{
    for (unsigned int index = 0; index != MeshDrawInfoList.size(); index++)
    {
        if (MeshDrawInfoList[index]->friendlyName == friendlyNameToFind)
        {
            return MeshDrawInfoList[index];
        }
    }

    std::cout << "Cannot find model info for the name provided : " << friendlyNameToFind << std::endl;

    return NULL;
}

sPhysicsProperties* cControlGameEngine::FindPhysicalModelByName(std::string modelName)
{
    for (unsigned int index = 0; index != PhysicsModelList.size(); index++)
    {
        if (PhysicsModelList[index]->modelName == modelName)
        {
            return PhysicsModelList[index];
        }
    }

    std::cout << "Cannot find physical mesh model for the name provided : " << modelName << std::endl;

    return NULL;
}

void cControlGameEngine::DrawObject(cMesh* currentMesh, glm::mat4 matModelParent, GLuint shaderProgramID)
{
    //--------------------------Calculate Matrix Model Transformation--------------------------------

    glm::mat4 matModel = matModelParent;

    glm::mat4 matTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(currentMesh->drawPosition.x, currentMesh->drawPosition.y, currentMesh->drawPosition.z));

    glm::mat4 matRotation = glm::mat4_cast(currentMesh->get_qOrientation());

    glm::mat4 matScale = glm::scale(glm::mat4(1.0f),
        glm::vec3(currentMesh->drawScale.x, currentMesh->drawScale.y, currentMesh->drawScale.z));

    matModel = matModel * matTranslate;

    matModel = matModel * matRotation;

    matModel = matModel * matScale;

    //-------------------------Get Model Info--------------------------------------------------------

    GLint matModel_UL = glGetUniformLocation(shaderProgramID, "matModel");
    glUniformMatrix4fv(matModel_UL, 1, GL_FALSE, glm::value_ptr(matModel));

    glm::mat4 matModel_InverseTranspose = glm::inverse(glm::transpose(matModel));

    GLint matModel_IT_UL = glGetUniformLocation(shaderProgramID, "matModel_IT");
    glUniformMatrix4fv(matModel_IT_UL, 1, GL_FALSE, glm::value_ptr(matModel_InverseTranspose));

    // ---------------------Check Light and Wireframe-------------------------------------------------

    GLint bDoNotLight_UL = glGetUniformLocation(shaderProgramID, "bDoNotLight");

    if (currentMesh->bIsWireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (currentMesh->bDoNotLight)
        glUniform1f(bDoNotLight_UL, (GLfloat)GL_TRUE);
    else
        glUniform1f(bDoNotLight_UL, (GLfloat)GL_FALSE);

    //-------------------------Get Manual Color from Shader----------------------------------------

    GLint bUseManualColour_UL = glGetUniformLocation(shaderProgramID, "bUseManualColour");

    if (currentMesh->bUseManualColours)
    {
        glUniform1f(bUseManualColour_UL, (GLfloat)GL_TRUE);

        GLint manualColourRGBA_UL = glGetUniformLocation(shaderProgramID, "manualColourRGBA");
        glUniform4f(manualColourRGBA_UL,
            currentMesh->wholeObjectManualColourRGBA.r,
            currentMesh->wholeObjectManualColourRGBA.g,
            currentMesh->wholeObjectManualColourRGBA.b,
            currentMesh->wholeObjectManualColourRGBA.a);
    }
    else
        glUniform1f(bUseManualColour_UL, (GLfloat)GL_FALSE);

    //------------------------------Add Textures----------------------------------------------
    
    GLint bUseDiscardMaskTexture_UL = glGetUniformLocation(shaderProgramID, "bUseDiscardMaskTexture");
    GLint bUseTextureColour_UL = glGetUniformLocation(shaderProgramID, "bUseTextureColour");

    if (currentMesh->bUseTextures)
    {
        GLint bUseHeightMap_UL = glGetUniformLocation(shaderProgramID, "bUseHeightMap");
        GLint textureMix_UL = glGetUniformLocation(shaderProgramID, "bUseTextureMixRatio");
        GLint bDiscardBlackAreas_UL = glGetUniformLocation(shaderProgramID, "bDiscardBlackInHeightMap");
        GLint bDiscardColoredAreas_UL = glGetUniformLocation(shaderProgramID, "bDiscardColoredAreasInHeightMap");
        
        glUniform1f(bDiscardBlackAreas_UL, (GLfloat)GL_FALSE);
        glUniform1f(bDiscardColoredAreas_UL, (GLfloat)GL_FALSE);
        glUniform1f(textureMix_UL, (GLfloat)GL_FALSE);
        glUniform1f(bUseHeightMap_UL, (GLfloat)GL_FALSE);
        glUniform1f(bUseDiscardMaskTexture_UL, (GLfloat)GL_FALSE);

        glUniform1f(bUseTextureColour_UL, (GLfloat)GL_TRUE);

        for (int textureIndex = 0; textureIndex < currentMesh->mTextureDetailsList.size(); textureIndex++)
        {
            //-----------------------------To apply Height Map to the texture--------------------------------------------

            if (currentMesh->mTextureDetailsList[textureIndex].bUseHeightMap)
            {
                //-----------------------Add Height Map texture Sampler---------------------------------------------

                GLuint textureToBeAdded = mTextureManager->getTextureIDFromName(currentMesh->mTextureDetailsList[textureIndex].texturePath);

                glActiveTexture(GL_TEXTURE0 + currentMesh->mTextureDetailsList[textureIndex].textureUnit);

                glBindTexture(GL_TEXTURE_2D, textureToBeAdded);

                GLint texture_HM_UL = glGetUniformLocation(shaderProgramID, "heightMapSampler");
                glUniform1i(texture_HM_UL, currentMesh->mTextureDetailsList[textureIndex].textureUnit);

                //--------------------Adjust scale and UVOffset of Height Map--------------------------------------

                glUniform1f(bUseHeightMap_UL, (GLfloat)GL_TRUE);

                GLint heightScale_UL = glGetUniformLocation(shaderProgramID, "heightScale");
                GLint UVOffset_UL = glGetUniformLocation(shaderProgramID, "UVOffset");

                glUniform1f(heightScale_UL, currentMesh->mTextureDetailsList[textureIndex].heightMapScale);

                glUniform2f(UVOffset_UL, currentMesh->mTextureDetailsList[textureIndex].UVOffset.x, currentMesh->mTextureDetailsList[textureIndex].UVOffset.y);

                //----------------Discard colored or uncolored areas of Height Map---------------------------------

                if (currentMesh->mTextureDetailsList[textureIndex].bDiscardBlackAreasInHeightMap)
                    glUniform1f(bDiscardBlackAreas_UL, (GLfloat)GL_TRUE);

                if (currentMesh->mTextureDetailsList[textureIndex].bDiscardColoredAreasInHeightMap)
                    glUniform1f(bDiscardColoredAreas_UL, (GLfloat)GL_TRUE);
            }

            else
            {
                //--------------------------------Apply normal texture-------------------------------------------------------

                GLuint textureToBeAdded = mTextureManager->getTextureIDFromName(currentMesh->mTextureDetailsList[textureIndex].texturePath);

                glActiveTexture(GL_TEXTURE0 + currentMesh->mTextureDetailsList[textureIndex].textureUnit);

                glBindTexture(GL_TEXTURE_2D, textureToBeAdded);

                GLint texture_00_UL = glGetUniformLocation(shaderProgramID, "textureAdder");
                glUniform1i(texture_00_UL, currentMesh->mTextureDetailsList[textureIndex].textureUnit);

                //--------------------------------To apply Discard Mask Texture-----------------------------------------------

                if (currentMesh->mTextureDetailsList[textureIndex].bUseDiscardMaskTexture)
                {
                    glUniform1f(bUseDiscardMaskTexture_UL, (GLfloat)GL_TRUE);

                    GLuint textureToBeMasked = mTextureManager->getTextureIDFromName(currentMesh->mTextureDetailsList[textureIndex].discardMaskTexturePath);

                    glActiveTexture(GL_TEXTURE0 + currentMesh->mTextureDetailsList[textureIndex].discardMaskTextureUnit);

                    glBindTexture(GL_TEXTURE_2D, textureToBeMasked);

                    GLint maskSampler_UL = glGetUniformLocation(shaderProgramID, "maskSampler");
                    glUniform1i(maskSampler_UL, currentMesh->mTextureDetailsList[textureIndex].discardMaskTextureUnit);
                }
            }
        }

        //--------------------------------To apply Blended Textures-----------------------------------------------------

        if(currentMesh->mMixedTextures.texturePathList.size() > 0)
        {
            GLint textureMixRatio_0_3_UL = glGetUniformLocation(shaderProgramID, "textureMixRatio_0_3");

            glUniform1f(textureMix_UL, (GLfloat)GL_TRUE);

            for (int texturesCount = 0; texturesCount < currentMesh->mMixedTextures.texturePathList.size(); texturesCount++)
            {
                GLuint textureToBeAdded = mTextureManager->getTextureIDFromName(currentMesh->mMixedTextures.texturePathList[texturesCount]);

                glActiveTexture(GL_TEXTURE0 + currentMesh->mMixedTextures.textureUnit[texturesCount]);

                glBindTexture(GL_TEXTURE_2D, textureToBeAdded);

                std::string textureSamplerName = "texture_0" + std::to_string(texturesCount);

                GLint texture_00_UL = glGetUniformLocation(shaderProgramID, textureSamplerName.c_str());
                glUniform1i(texture_00_UL, currentMesh->mMixedTextures.textureUnit[texturesCount]);
            }

            // Adding texture ratios
            glUniform4f(textureMixRatio_0_3_UL,
                currentMesh->mMixedTextures.textureRatiosList[0],
                currentMesh->mMixedTextures.textureRatiosList[1],
                currentMesh->mMixedTextures.textureRatiosList[2],
                currentMesh->mMixedTextures.textureRatiosList[3]);
        }
    }

    else
        glUniform1f(bUseTextureColour_UL, (GLfloat)GL_FALSE);

    //-----------------------------------Setup Cube Map----------------------------------------------
    if (cubeMapManager != NULL)
    {
        GLuint skyBoxID = mTextureManager->getTextureIDFromName(cubeMapManager->GetSkyBoxName());

        glActiveTexture(GL_TEXTURE0 + cubeMapManager->GetSkyBoxTextureUnit());

        glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxID);

        GLint cubeMapSampler_UL = glGetUniformLocation(shaderProgramID, "cubeMapSampler");
        glUniform1i(cubeMapSampler_UL, cubeMapManager->GetSkyBoxTextureUnit());
    }

    //-------------------------Check Alpha transparency----------------------------------------

    if (currentMesh->alphaTransparency < 1.0f) // Transparent
    {
        glUniform1f(bUseDiscardMaskTexture_UL, (GLfloat)GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else // Opaque
        glDisable(GL_BLEND);

    GLint aplhaTransparency_UL = glGetUniformLocation(shaderProgramID, "alphaTransparency");
    glUniform1f(aplhaTransparency_UL, currentMesh->alphaTransparency);

    //-------------------------Find Model Info and Draw----------------------------------------

    sModelDrawInfo modelInfo;

    if (mVAOManager->FindDrawInfoByModelName(currentMesh->friendlyName, modelInfo))
    {
        glBindVertexArray(modelInfo.VAO_ID);
        glDrawElements(GL_TRIANGLES,
            modelInfo.numberOfIndices,
            GL_UNSIGNED_INT,
            0);
        glBindVertexArray(0);
    }

    //------------------------------Remove Scaling---------------------------------------------

    glm::mat4 matRemoveScaling = glm::scale(glm::mat4(1.0f),
        glm::vec3(1.0f / currentMesh->drawScale.x, 1.0f / currentMesh->drawScale.y, 1.0f / currentMesh->drawScale.z));

    matModel = matModel * matRemoveScaling;

    return;
}

int cControlGameEngine::InitializeShader()
{
    mShaderManager = new cShaderManager();

    mShaderManager->setBasePath("Assets/Shaders");

    vertexShader.fileName = "vertexShaderWithUV.glsl";

    fragmentShader.fileName = "fragmentShaderWithUV.glsl";

    if (!mShaderManager->createProgramFromFile("shader01", vertexShader, fragmentShader))
    {
        std::cout << "ERROR: UNABLE TO COMPILE/LINK SHADER" << std::endl;
        std::cout << mShaderManager->getLastError();
        return -1;
    }

    shaderProgramID = mShaderManager->getIDFromFriendlyName("shader01");

    return 0;
}

//---------------------------------------------------Public Functions-----------------------------------------------------------------------

//--------------------------------------COMMON FUNCTIONS---------------------------------------------------------------

float cControlGameEngine::getRandomFloat(float num1, float num2)
{
    float randomNum = (static_cast<float>(rand())) / (static_cast<float>(RAND_MAX));
    float difference = num2 - num1;
    float product = randomNum * difference;

    return num1 + product;
}

//--------------------------------------CAMERA CONTROLS----------------------------------------------------------------

void cControlGameEngine::MoveCameraPosition(float translate_x, float translate_y, float translate_z)
{
    cameraEye = glm::vec3(translate_x, translate_y, translate_z);
}

void cControlGameEngine::MoveCameraTarget(float translate_x, float translate_y, float translate_z)
{
    cameraTarget = glm::vec3(translate_x, translate_y, translate_z);
}

glm::vec3 cControlGameEngine::GetCurrentCameraPosition()
{
    return cameraEye;
}

glm::vec3 cControlGameEngine::GetCurrentCameraTarget()
{
    return cameraTarget;
}

//--------------------------------------MESH CONTROLS-----------------------------------------------------------------

void cControlGameEngine::LoadModelsInto3DSpace(std::string filePath, std::string modelName, float initial_x, float initial_y, float initial_z)
{
    sModelDrawInfo* newModel = new sModelDrawInfo;

    cMesh* newMesh = new cMesh();

    bool result = mVAOManager->LoadModelIntoVAO(modelName, filePath, *newModel, shaderProgramID);

    if (!result)
    {
        std::cout << "Cannot load model - " << modelName << std::endl;
        return;
    }

    MeshDrawInfoList.push_back(newModel);

    newMesh->meshName = filePath;

    newMesh->friendlyName = modelName;

    newMesh->drawPosition = glm::vec3(initial_x, initial_y, initial_z);

    std::cout << "Loaded: " << newMesh->friendlyName << " | Vertices : " << newModel->numberOfVertices << std::endl;

    TotalMeshList.push_back(newMesh);
}

void cControlGameEngine::ChangeColor(std::string modelName, float r, float g, float b)
{
    cMesh* meshToBeScaled = g_pFindMeshByFriendlyName(modelName);

    meshToBeScaled->wholeObjectManualColourRGBA = glm::vec4(r, g, b, 1.0f);
}

void cControlGameEngine::UseManualColors(std::string modelName, bool useColor)
{
    cMesh* meshToBeScaled = g_pFindMeshByFriendlyName(modelName);

    if (useColor)
        meshToBeScaled->bUseManualColours = true;
    else
        meshToBeScaled->bUseManualColours = false;
}

void cControlGameEngine::ScaleModel(std::string modelName, float scale_value)
{
    cMesh* meshToBeScaled = g_pFindMeshByFriendlyName(modelName);

    meshToBeScaled->setUniformDrawScale(scale_value);
}

void cControlGameEngine::MoveModel(std::string modelName, float translate_x, float translate_y, float translate_z)
{
    cMesh* meshToBeTranslated = g_pFindMeshByFriendlyName(modelName);

    const glm::vec3& position = glm::vec3(translate_x, translate_y, translate_z);

    meshToBeTranslated->setDrawPosition(position);
}

glm::vec3 cControlGameEngine::GetModelPosition(std::string modelName)
{
    cMesh* meshPosition = g_pFindMeshByFriendlyName(modelName);

    return meshPosition->getDrawPosition();
}

float cControlGameEngine::GetModelScaleValue(std::string modelName)
{
    cMesh* meshScaleValue = g_pFindMeshByFriendlyName(modelName);

    return meshScaleValue->drawScale.x;
}

void cControlGameEngine::RotateMeshModel(std::string modelName, float angleDegrees, float rotate_x, float rotate_y, float rotate_z)
{
    cMesh* meshToBeRotated = g_pFindMeshByFriendlyName(modelName);

    float radianVal = glm::radians(angleDegrees);

    glm::quat rotationQuaternion = glm::angleAxis(radianVal, glm::vec3(rotate_x, rotate_y, rotate_z));

    if (meshToBeRotated->drawOrientation != glm::vec3(0.f))
        rotationQuaternion *= meshToBeRotated->get_qOrientation();
        
    meshToBeRotated->setDrawOrientation(rotationQuaternion, glm::vec3(rotate_x * angleDegrees, rotate_y * angleDegrees, rotate_z * angleDegrees));

    //meshToBeRotated->setDrawOrientation(rotation);

    //cMesh* meshToBeRotated = g_pFindMeshByFriendlyName(modelName);

    //glm::vec3 eulerAngles(rotate_x, rotate_y, rotate_z);

    //// Converting Euler angles to quaternion
    //glm::quat rotationQuaternion = glm::quat(glm::radians(eulerAngles));

    //if (meshToBeRotated->drawOrientation != glm::vec3(0.f))
    //    rotationQuaternion *= meshToBeRotated->get_qOrientation();
}

void cControlGameEngine::TurnVisibilityOn(std::string modelName)
{
    cMesh* meshVisibility = g_pFindMeshByFriendlyName(modelName);

    if (meshVisibility->bIsVisible != true)
        meshVisibility->bIsVisible = true;
    else
        meshVisibility->bIsVisible = false;
}

void cControlGameEngine::TurnWireframeModeOn(std::string modelName)
{
    cMesh* meshWireframe = g_pFindMeshByFriendlyName(modelName);

    if (meshWireframe->bIsWireframe == true)
        meshWireframe->bIsWireframe = false;
    else
        meshWireframe->bIsWireframe = true;
}

void cControlGameEngine::TurnMeshLightsOn(std::string modelName)
{
    cMesh* meshLights = g_pFindMeshByFriendlyName(modelName);

    if (meshLights->bDoNotLight == true)
        meshLights->bDoNotLight = false;
    else
        meshLights->bDoNotLight = true;
}

void cControlGameEngine::DeleteMesh(std::string modelName)
{
    cMesh* meshModel = g_pFindMeshByFriendlyName(modelName);

    sPhysicsProperties* physicalModel = FindPhysicalModelByName(modelName);

    sModelDrawInfo* modelInfo = g_pFindModelInfoByFriendlyName(modelName);

    if (meshModel != NULL)
        TotalMeshList.erase(std::remove(TotalMeshList.begin(), TotalMeshList.end(), meshModel), TotalMeshList.end());

    if (physicalModel != NULL)
        PhysicsModelList.erase(std::remove(PhysicsModelList.begin(), PhysicsModelList.end(), physicalModel), PhysicsModelList.end());

    if (modelInfo != NULL)
        MeshDrawInfoList.erase(std::remove(MeshDrawInfoList.begin(), MeshDrawInfoList.end(), modelInfo), MeshDrawInfoList.end());
}

void cControlGameEngine::ShiftToNextMeshInList()
{
    meshListIndex++;

    if (meshListIndex == TotalMeshList.size())
        meshListIndex = 0;

    cMesh* existingMeshModel = TotalMeshList[meshListIndex];

    while (existingMeshModel->bIsAABBMesh)
    {
        meshListIndex++;

        if (meshListIndex == TotalMeshList.size())
            meshListIndex = 0;

        existingMeshModel = TotalMeshList[meshListIndex];
    }
}

void cControlGameEngine::ShiftToPreviousMeshInList()
{
    meshListIndex--;

    if (meshListIndex < 0)
        meshListIndex = int(TotalMeshList.size()) - 1;

    cMesh* existingMeshModel = TotalMeshList[meshListIndex];

    while (existingMeshModel->bIsAABBMesh)
    {
        meshListIndex--;

        if (meshListIndex < 0)
            meshListIndex = int(TotalMeshList.size()) - 1;

        existingMeshModel = TotalMeshList[meshListIndex];
    }
}

cMesh* cControlGameEngine::GetCurrentModelSelected()
{
    return TotalMeshList[meshListIndex];
}

void cControlGameEngine::SortMeshesBasedOnTransparency()
{
    glm::vec3 cameraPos = cameraEye;
    glm::vec3 cameraPerspective = cameraTarget;
    glm::vec3 cameraDirection = glm::normalize(cameraPerspective - cameraPos);

    float offsetVal = 5'000.0f;

    // Making position of skyBox 5'000.0f away from the view of the camera
    glm::vec3 skyBoxPos = cameraPos + offsetVal * cameraDirection;

    std::sort(TotalMeshList.begin(), TotalMeshList.end(), [cameraPos, skyBoxPos](cMesh* mesh1, cMesh* mesh2) {
       
        if (mesh1->bIsSkyBox)
            return (glm::distance(skyBoxPos, cameraPos) > glm::distance(mesh2->drawPosition, cameraPos));
        else if (mesh2->bIsSkyBox)
            return (glm::distance(mesh1->drawPosition, cameraPos) > glm::distance(skyBoxPos, cameraPos));
        else
            return (glm::distance(mesh1->drawPosition, cameraPos) > glm::distance(mesh2->drawPosition,cameraPos));
    });
}

void cControlGameEngine::MakeMeshAABB(std::string modelName, bool isAABB)
{
    cMesh* meshAABB = g_pFindMeshByFriendlyName(modelName);

    meshAABB->bIsAABBMesh = isAABB;
}

//-------------------------------------TEXTURE CONTROLS-----------------------------------------------------------------

void cControlGameEngine::UseTextures(std::string modelName, bool applyTexture)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    meshTexture->bUseTextures = applyTexture;
}

void cControlGameEngine::ApplyHeightMap(std::string modelName, std::string textureName, bool applyHeightMap)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
    {
        if (meshTexture->mTextureDetailsList[index].textureName == textureName)
        {
            meshTexture->mTextureDetailsList[index].bUseHeightMap = applyHeightMap;
            
            break;
        }
    }
}

void cControlGameEngine::AddTexturesToOverlap(std::string modelName, std::string texturePath, std::string textureName)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    if (mTextureManager->Create2DTextureFromBMPFile(texturePath, true))
    {
        std::cout << "Texture loaded successfully - [" << texturePath << "]" << std::endl;

        sMeshTexture newTexture;

        newTexture.texturePath = texturePath;
        newTexture.textureName = textureName;
        newTexture.textureUnit = textureUnitIndex;

        meshTexture->mTextureDetailsList.push_back(newTexture);

        // Incrementing the Texture Unit 
        textureUnitIndex++;

        if (textureUnitIndex >= 30) // Maintaining 30 as the threshold for the Texture Unit count
            textureUnitIndex = 0;
    }

    else
        std::cout << "ERROR : Loading texture failed - [" << texturePath << "]" << std::endl;
}

void cControlGameEngine::AddTexturesToTheMix(std::string modelName, std::string texturePath, std::string textureName, float textureRatio)
{ 
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    if (mTextureManager->Create2DTextureFromBMPFile(texturePath, true))
    {
        std::cout << "Texture loaded successfully - [" << texturePath << "]" << std::endl;

        sMeshTexture newTexture;

        meshTexture->mMixedTextures.texturePathList.push_back(texturePath);
        meshTexture->mMixedTextures.textureNameList.push_back(textureName);
        meshTexture->mMixedTextures.textureUnit.push_back(textureUnitIndex);
        meshTexture->mMixedTextures.textureRatiosList[meshTexture->mMixedTextures.textureNameList.size() - 1] = textureRatio;

        // Incrementing the Texture Unit 
        textureUnitIndex++;

        if (textureUnitIndex >= 30) // Maintaining 30 as the threshold for the Texture Unit count
            textureUnitIndex = 0;
    }

    else
        std::cout << "ERROR : Loading texture failed - [" << texturePath << "]" << std::endl;
}

void cControlGameEngine::AddDiscardMaskTexture(std::string modelName, std::string textureName, std::string discardMaskTexturePath)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    if (mTextureManager->Create2DTextureFromBMPFile(discardMaskTexturePath, true))
    {
        std::cout << "Texture loaded successfully - [" << discardMaskTexturePath << "]" << std::endl;

        for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
        {
            if (meshTexture->mTextureDetailsList[index].textureName == textureName)
            {
                meshTexture->mTextureDetailsList[index].discardMaskTexturePath = discardMaskTexturePath;
                meshTexture->mTextureDetailsList[index].discardMaskTextureUnit = textureUnitIndex;

                break;
            }
        }

        // Incrementing the Texture Unit 
        textureUnitIndex++;

        if (textureUnitIndex >= 30) // Maintaining 30 as the threshold for the Texture Unit count
            textureUnitIndex = 0;

    }

    else
        std::cout << "ERROR : Loading texture failed - [" << discardMaskTexturePath << "]" << std::endl;
}

void cControlGameEngine::ChangeTextureRatios(std::string modelName, std::vector <float> textureRatio)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    int MIXTURE_THRESHOLD = 4;
    // Maintaining a mixture threshold of 4 for now
    if (meshTexture->mMixedTextures.textureNameList.size() > 0)
    {
        for (int index = 0; index < MIXTURE_THRESHOLD; index++)
            meshTexture->mMixedTextures.textureRatiosList[index] = textureRatio[index];
    }
}

void cControlGameEngine::AdjustHeightMapScale(std::string modelName, std::string textureName, float height)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
    {
        if (meshTexture->mTextureDetailsList[index].textureName == textureName)
        {
            meshTexture->mTextureDetailsList[index].heightMapScale = height;

            break;
        }
    }
}

void cControlGameEngine::AdjustHeightMapUVPos(std::string modelName, std::string textureName, glm::vec2 UVOffsetPos)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
    {
        if (meshTexture->mTextureDetailsList[index].textureName == textureName)
        {
            meshTexture->mTextureDetailsList[index].UVOffset.x += UVOffsetPos.x;
            meshTexture->mTextureDetailsList[index].UVOffset.y += UVOffsetPos.y;

            break;
        }
    }
}

void cControlGameEngine::RemoveBlackAreasInHeightMap(std::string modelName, std::string textureName, bool removeBlackAreas)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
    {
        if (meshTexture->mTextureDetailsList[index].textureName == textureName)
        {
            meshTexture->mTextureDetailsList[index].bDiscardBlackAreasInHeightMap = removeBlackAreas;

            break;
        }
    }
}

void cControlGameEngine::RemoveColoredAreasHeightMap(std::string modelName, std::string textureName, bool removeColoredAreas)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);
    for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
    {
        if (meshTexture->mTextureDetailsList[index].textureName == textureName)
        {
            meshTexture->mTextureDetailsList[index].bDiscardColoredAreasInHeightMap = removeColoredAreas;

            break;
        }
    }
}

void cControlGameEngine::ApplyDiscardMaskTexture(std::string modelName, std::string textureName, bool applyDiscardMask)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    for (int index = 0; index < meshTexture->mTextureDetailsList.size(); index++)
    {
        if (meshTexture->mTextureDetailsList[index].textureName == textureName)
        {
            meshTexture->mTextureDetailsList[index].bUseDiscardMaskTexture = applyDiscardMask;

            break;
        }
    }
}

void cControlGameEngine::AdjustAlphaTransparency(std::string modelName, float alphaTransparencyLevel)
{
    cMesh* meshTexture = g_pFindMeshByFriendlyName(modelName);

    meshTexture->alphaTransparency = alphaTransparencyLevel;

    if (alphaTransparencyLevel)
    {
        if (!bTransparencyMeshAvailable)
            bTransparencyMeshAvailable = true;
    }
    else
    {
        bool bTransparencyMesh = false;

        for (cMesh* meshTransparency : TotalMeshList)
        {
            if (meshTransparency->alphaTransparency < 1.0f)
                bTransparencyMesh = true;
        }

        if (!bTransparencyMesh && bTransparencyMeshAvailable)
            bTransparencyMeshAvailable = false;
    }
}

void cControlGameEngine::UseDiscardMaskTexture(GLuint shaderProgramID, GLint bUseDiscardMaskTexture_UL, std::string texturePath, int textureUnit)
{
    glUniform1f(bUseDiscardMaskTexture_UL, (GLfloat)GL_TRUE);

    GLuint textureToBeMasked = mTextureManager->getTextureIDFromName(texturePath);

    glActiveTexture(GL_TEXTURE0 + textureUnit);

    glBindTexture(GL_TEXTURE_2D, textureToBeMasked);

    GLint maskSampler_UL = glGetUniformLocation(shaderProgramID, "maskSampler");
    glUniform1i(maskSampler_UL, textureUnit);
}

//-------------------------------------CUBE MAP CONTROLS----------------------------------------------------------------

bool cControlGameEngine::AddCubeMap(std::string modelName, std::string skyBoxName, std::string filePathPosX, std::string filePathNegX, std::string filePathPosY,
                                    std::string filePathNegY, std::string filePathPosZ, std::string filePathNegZ)
{
    std::string errors;
    bool result;

    mTextureManager->SetBasePath("Assets/Textures/CubeMaps");
   
    result = mTextureManager->CreateCubeTextureFromBMPFiles(skyBoxName, filePathPosX, filePathNegX, filePathPosY, filePathNegY, filePathPosZ,
                                                    filePathNegZ, true, errors);

    mTextureManager->SetBasePath("Assets/Textures");

    if (result)
    {
        cubeMapManager = new cCubeMap();

        cubeMapManager->AddSkyBoxName(skyBoxName);
        cubeMapManager->AddMeshModelName(modelName);
        cubeMapManager->AddSkyBoxTextureUnit(textureUnitIndex);
        cubeMapManager->AddSkyBoxFilePaths(filePathPosX, filePathNegX, filePathPosY, filePathNegY, filePathPosZ, filePathNegZ);

        cMesh* meshSkyBox = g_pFindMeshByFriendlyName(modelName);

        meshSkyBox->bIsSkyBox = true;

        // Incrementing the Texture Unit 
        textureUnitIndex++;

        if (textureUnitIndex >= 30) // Maintaining 30 as the threshold for the Texture Unit count
            textureUnitIndex = 0;

        return 0;
    }
    else
    {
        std::cout << "Error loading Cube Map : " << errors << std::endl;
        return -1;
    }
}

void cControlGameEngine::DeleteCubeMap()
{
    if (cubeMapManager != NULL)
    {
        delete cubeMapManager;

        for (cMesh* skyBox : TotalMeshList)
        {
            if (skyBox->bIsSkyBox)
                DeleteMesh(skyBox->meshName);
        }
    }
}

void cControlGameEngine::DrawSkyBox(cMesh* skyBox)
{
    //------------------------------Make DiscardMask false---------------------------------------

    GLint bUseDiscardMaskTexture_UL = glGetUniformLocation(shaderProgramID, "bUseDiscardMaskTexture");

    glUniform1f(bUseDiscardMaskTexture_UL, (GLfloat)GL_FALSE);

    //------------------------------Draw SkyBox last----------------------------------------------

    //ScaleModel(cubeMapManager->GetMeshModelName(), 10.0f);
    //MoveModel(cubeMapManager->GetMeshModelName(), 0.0f, 100.0f, 0.0f);

    ScaleModel(cubeMapManager->GetMeshModelName(), 5'000.0f);
    MoveModel(cubeMapManager->GetMeshModelName(), cameraEye.x, cameraEye.y, cameraEye.z);

    GLint bApplySkyBox_UL = glGetUniformLocation(shaderProgramID, "bApplySkyBox");
    glUniform1f(bApplySkyBox_UL, (GLfloat)GL_TRUE);

    // Making mesh normals to face inward instead of outward
    glCullFace(GL_FRONT);

    DrawObject(skyBox, glm::mat4(1.0f), shaderProgramID);

    glUniform1f(bApplySkyBox_UL, (GLfloat)GL_FALSE);

    // Making normals face outward
    glCullFace(GL_BACK);
}

//--------------------------------------LIGHTS CONTROLS-----------------------------------------------------------------

void cControlGameEngine::CreateLight(int lightId, float initial_x, float initial_y, float initial_z)
{
    if (lightId > 15)
    {
        std::cout << "Light Id is more than 15 ! Cannot create light !" << std::endl;
        return;
    }
    std::cout << "Light : " << lightId << " Created !" << std::endl;

    mLightManager->SetUniformLocations(shaderProgramID, lightId);

    mLightManager->theLights[lightId].param2.x = 1.0f; // Turn on

    mLightManager->theLights[lightId].param1.x = 2.0f;   // 0 = point light , 1 = spot light , 2 = directional light

    mLightManager->theLights[lightId].param1.y = 50.0f; // inner angle

    mLightManager->theLights[lightId].param1.z = 50.0f; // outer angle

    mLightManager->theLights[lightId].position.x = initial_x;

    mLightManager->theLights[lightId].position.y = initial_y;

    mLightManager->theLights[lightId].position.z = initial_z;

    mLightManager->theLights[lightId].direction = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);

    mLightManager->theLights[lightId].atten.x = 0.0f;        // Constant attenuation

    mLightManager->theLights[lightId].atten.y = 0.1f;        // Linear attenuation

    mLightManager->theLights[lightId].atten.z = 0.0f;        // Quadratic attenuation
}

void cControlGameEngine::TurnOffLight(int lightId, bool turnOff)
{
    if (turnOff)
        mLightManager->theLights[lightId].param2.x = 0.0f;
    else
        mLightManager->theLights[lightId].param2.x = 1.0f;
}

void cControlGameEngine::PositionLight(int lightId, float translate_x, float translate_y, float translate_z)
{
    mLightManager->theLights[lightId].position.x = translate_x;

    mLightManager->theLights[lightId].position.y = translate_y;

    mLightManager->theLights[lightId].position.z = translate_z;
}

void cControlGameEngine::ChangeLightIntensity(int lightId, float linearAttentuation, float quadraticAttentuation)
{
    mLightManager->theLights[lightId].atten.y = linearAttentuation;

    mLightManager->theLights[lightId].atten.z = quadraticAttentuation;
}

void cControlGameEngine::ChangeLightType(int lightId, float lightType)
{
    mLightManager->theLights[lightId].param1.x = lightType;
}

void cControlGameEngine::ChangeLightAngle(int lightId, float innerAngle, float outerAngle)
{
    mLightManager->theLights[lightId].param1.y = innerAngle; // inner angle

    mLightManager->theLights[lightId].param1.z = outerAngle; // outer angle
}

void cControlGameEngine::ChangeLightDirection(int lightId, float direction_x, float direction_y, float direction_z)
{
    mLightManager->theLights[lightId].direction = glm::vec4(direction_x, direction_y, direction_z, 1.0f);

}

void cControlGameEngine::ChangeLightColour(int lightId, float color_r, float color_g, float color_b)
{
    mLightManager->theLights[lightId].diffuse = glm::vec4(color_r, color_g, color_b, 1.0f);
}

glm::vec3 cControlGameEngine::GetLightPosition(int lightId)
{
    return mLightManager->theLights[lightId].position;
}

glm::vec3 cControlGameEngine::GetLightDirection(int lightId)
{
    return mLightManager->theLights[lightId].direction;
}

float cControlGameEngine::GetLightLinearAttenuation(int lightId)
{
    return mLightManager->theLights[lightId].atten.y;
}

float cControlGameEngine::GetLightQuadraticAttenuation(int lightId)
{
    return mLightManager->theLights[lightId].atten.z;
}

float cControlGameEngine::GetLightType(int lightId)
{
    return mLightManager->theLights[lightId].param1.x;
}

float cControlGameEngine::GetLightInnerAngle(int lightId)
{
    return mLightManager->theLights[lightId].param1.y;
}

float cControlGameEngine::GetLightOuterAngle(int lightId)
{
    return mLightManager->theLights[lightId].param1.z;
}

glm::vec3 cControlGameEngine::GetLightColor(int lightId)
{
    return mLightManager->theLights[lightId].diffuse;
}

float cControlGameEngine::IsLightOn(int lightId)
{
    return mLightManager->theLights[lightId].param2.x;
}

void cControlGameEngine::ShiftToNextLightInList()
{
    lightListIndex++;

    if (lightListIndex > 10)
        lightListIndex = 0;
}

int cControlGameEngine::GetCurrentLightSelected()
{
    return lightListIndex;
}

//--------------------------------------PHYSICS CONTROLS---------------------------------------------------------------

void  cControlGameEngine::ComparePhysicalAttributesWithOtherModels()
{
    for (int physicalModelCount = 0; physicalModelCount < PhysicsModelList.size(); physicalModelCount++)
    {
        if (PhysicsModelList[physicalModelCount]->physicsMeshType == "Sphere")
        {
            sPhysicsProperties* spherePhysicalModel = FindPhysicalModelByName(PhysicsModelList[physicalModelCount]->modelName);

            if (spherePhysicalModel != NULL)
            {
                for (int anotherModelCount = 0; anotherModelCount < PhysicsModelList.size(); anotherModelCount++)
                {
                    if (anotherModelCount == physicalModelCount)
                        continue;
                    else
                    {
                        if (PhysicsModelList[anotherModelCount]->physicsMeshType == "Plane" || PhysicsModelList[anotherModelCount]->physicsMeshType == "Box")
                            MakePhysicsHappen(spherePhysicalModel, PhysicsModelList[anotherModelCount]->modelName, PhysicsModelList[anotherModelCount]->physicsMeshType);

                        else if (PhysicsModelList[anotherModelCount]->physicsMeshType == "Sphere")
                            MakePhysicsHappen(spherePhysicalModel, PhysicsModelList[anotherModelCount]->modelName, PhysicsModelList[anotherModelCount]->physicsMeshType);
                    }
                }               
            }
        }
    }
}

void cControlGameEngine::ResetPosition(sPhysicsProperties* physicsModel)
{
    physicsModel->position.x = getRandomFloat(0.0, 20.0);
    physicsModel->position.y = getRandomFloat(100.0, 150.0);
    physicsModel->position.z = getRandomFloat(0.0, 20.0);
    physicsModel->sphereProps->velocity = glm::vec3(0.0f, -getRandomFloat(1.0, 5.0), 0.0f);

    ChangeColor(physicsModel->modelName, 1.0, 1.0, 1.0); //Reseting spheres to white again
}

void cControlGameEngine::PhysicsEulerIntegration(std::string modelName)
{
    sPhysicsProperties* physicsModel = FindPhysicalModelByName(modelName);
    cMesh* meshObject = g_pFindMeshByFriendlyName(modelName);

    //--------Calculate acceleration & velocity(Euler forward integration step)-------------------

    mPhysicsManager->EulerForwardIntegration(physicsModel, deltaTime);

    //------------------Set objects's position based on new velocity-------------------------------

    meshObject->setDrawPosition(physicsModel->position);
}

void cControlGameEngine::ComparePhysicsObjectWithAABBs(std::string modelName)
{
    bool result = false;

    sPhysicsProperties* physicsModel = FindPhysicalModelByName(modelName);
    cMesh* meshObject = g_pFindMeshByFriendlyName(modelName);

    for (cAABB* boundingBox : AABBList)
    {
        if (boundingBox->IsInsideAABB(physicsModel->position))
        {
            //---------------------------Change AABB color to green---------------------------------

            if (!boundingBox->objectInsideAABB)
            {
                ChangeColor(boundingBox->meshModelName, 0.1, 0.85, 0.1);
                boundingBox->objectInsideAABB = true;
            }

            //------------------------Check for collision with triangles----------------------------

            result = mPhysicsManager->CheckForAABBTriangleCollision(boundingBox->trianglesInsideAABBList, physicsModel);

            if (result)
            {
                mPhysicsManager->PlaneCollisionResponse(physicsModel, deltaTime);

                //-------------------Set sphere's position after response-------------------------------

                meshObject->setDrawPosition(physicsModel->position);
            }
        }

        else
        {
            //---------------------------Change AABB color to red------------------------------------

            if (boundingBox->objectInsideAABB)
            {
                ChangeColor(boundingBox->meshModelName, 0.90f, 0.1f, 0.1f);
                boundingBox->objectInsideAABB = false;
            }
        }
    }
}

void cControlGameEngine::MakePhysicsHappen(sPhysicsProperties* physicsModel, std::string model2Name, std::string collisionType)
{
    //--------Calculate acceleration & velocity(Euler forward integration step)-------------------
    
    mPhysicsManager->EulerForwardIntegration(physicsModel, deltaTime);
    
    //----------------Reset model position once it reaches threshold------------------------------
    
    if (physicsModel->position.y < - 200.0f)
        ResetPosition(physicsModel);

    //---------------------Get Mesh models--------------------------------------------------------
    
    cMesh* sphereMesh = g_pFindMeshByFriendlyName(physicsModel->modelName);
    cMesh* model2Mesh = g_pFindMeshByFriendlyName(model2Name);
    
    //------------------Set sphere's position based on new velocity-------------------------------

    sphereMesh->setDrawPosition(physicsModel->position);

    //----------------------------Check for Collision---------------------------------------------

    bool result = false;

    /*if (physicsModel->position.x < model2Mesh->drawPosition.x + 70.0f && physicsModel->position.x > model2Mesh->drawPosition.x - 70.0f &&
        physicsModel->position.y < model2Mesh->drawPosition.y + 70.0f && physicsModel->position.y > model2Mesh->drawPosition.y - 70.0f &&
        physicsModel->position.z < model2Mesh->drawPosition.z + 200.0f && physicsModel->position.z > model2Mesh->drawPosition.z - 200.0f)
    {
    }*/

    if (collisionType == "Plane" || collisionType == "Box")
    {
        sModelDrawInfo* modelInfo = g_pFindModelInfoByFriendlyName(model2Name);

        //------------------------Plane Collision Check---------------------------------------------
        
        if(model2Mesh != NULL && modelInfo!= NULL)
            result = mPhysicsManager->CheckForPlaneCollision(modelInfo, model2Mesh, physicsModel);

        if (result)
        {
            mPhysicsManager->PlaneCollisionResponse(physicsModel, deltaTime);

            //-------------------Set sphere's position after response-------------------------------

            sphereMesh->setDrawPosition(physicsModel->position);
        }
    }

    else if (collisionType == "Sphere")
    {
        //---------------------Get second sphere's physical model-----------------------------------

        sPhysicsProperties * secondSphereModel = FindPhysicalModelByName(model2Name);

        //----------------------Sphere Collision Check----------------------------------------------
        
        if (secondSphereModel != NULL)
            result = mPhysicsManager->CheckForSphereCollision(physicsModel, secondSphereModel);

        if (result)
        {
            mPhysicsManager->SphereCollisionResponse(physicsModel, secondSphereModel);

            //------------------------Change colors after collision-----------------------------

            ChangeColor(physicsModel->modelName, getRandomFloat(0.0, 0.50), getRandomFloat(0.0, 0.50), getRandomFloat(0.0, 0.50));
            ChangeColor(secondSphereModel->modelName, getRandomFloat(0.0, 0.50), getRandomFloat(0.0, 0.50), getRandomFloat(0.0, 0.50));

            //-------------------Set sphere's position after response---------------------------

            sphereMesh->setDrawPosition(physicsModel->position);
            model2Mesh->setDrawPosition(secondSphereModel->position);
        }
    }
}

void cControlGameEngine::AddSpherePhysicsToMesh(std::string modelName, std::string physicsMeshType, float objectRadius)
{
    sPhysicsProperties* newPhysicsModel = new sPhysicsProperties(physicsMeshType);

    cMesh* meshDetails = g_pFindMeshByFriendlyName(modelName);

    glm::vec3 modelPosition = meshDetails->getDrawPosition();

    newPhysicsModel->physicsMeshType = physicsMeshType;

    newPhysicsModel->modelName = modelName;

    newPhysicsModel->sphereProps->radius = objectRadius;

    newPhysicsModel->position = modelPosition;

    PhysicsModelList.push_back(newPhysicsModel);
}

void cControlGameEngine::AddPlanePhysicsToMesh(std::string modelName, std::string physicsMeshType)
{
    sPhysicsProperties* newPhysicsModel = new sPhysicsProperties(physicsMeshType);

    cMesh* meshDetails = g_pFindMeshByFriendlyName(modelName);

    glm::vec3 modelPosition = meshDetails->getDrawPosition();

    newPhysicsModel->physicsMeshType = physicsMeshType;

    newPhysicsModel->modelName = modelName;

    newPhysicsModel->position = modelPosition;

    PhysicsModelList.push_back(newPhysicsModel);
}

void cControlGameEngine::ChangeModelPhysicsPosition(std::string modelName, float newPositionX, float newPositionY, float newPositionZ)
{
    sPhysicsProperties* physicalModelFound = FindPhysicalModelByName(modelName);

    physicalModelFound->position.x = newPositionX;
    physicalModelFound->position.y = newPositionY;
    physicalModelFound->position.z = newPositionZ;
}

void cControlGameEngine::ChangeModelPhysicsVelocity(std::string modelName, glm::vec3 velocityChange)
{
    sPhysicsProperties* physicalModelFound = FindPhysicalModelByName(modelName);

    physicalModelFound->sphereProps->velocity = velocityChange;
}

void cControlGameEngine::ChangeModelPhysicsAcceleration(std::string modelName, glm::vec3 accelerationChange)
{
    sPhysicsProperties* physicalModelFound = FindPhysicalModelByName(modelName);

    physicalModelFound->sphereProps->acceleration = accelerationChange;
}

int cControlGameEngine::ChangeModelPhysicalMass(std::string modelName, float mass)
{
    sPhysicsProperties* physicalModelFound = FindPhysicalModelByName(modelName);

    if (mass > 0.0f)
    {
        physicalModelFound->sphereProps->inverse_mass = 1.0 / mass;
        return 0;
    }
    
    return 1;
}

//----------------------------------------AABB CONTROLS-----------------------------------------------------------------

void cControlGameEngine::CreateSceneAABBs(glm::vec3 startingPoint, glm::ivec3 cubesCount, float cubeSize, std::string meshFilePath)
{
    for (int xPosOffset = 0; xPosOffset < cubesCount.x; xPosOffset++)
    {
        for (int yPosOffset = 0; yPosOffset < cubesCount.y; yPosOffset++)
        {
            for (int zPosOffset = 0; zPosOffset < cubesCount.z; zPosOffset++)
            {
                //-------------------------------------Create AABB Mesh to Debug-------------------------------------------

                std::cout << "Cube Size : " << cubeSize << std::endl;

                float xPos = (xPosOffset * cubeSize) + startingPoint.x;
                float yPos = (yPosOffset * cubeSize) + startingPoint.y;
                float zPos = (zPosOffset * cubeSize) + startingPoint.z;

                std::string modelName = "Cube_" + std::to_string((int)xPos) + "_" + std::to_string((int)yPos) + "_" + std::to_string((int)zPos);

                LoadModelsInto3DSpace(meshFilePath, modelName, xPos, yPos, zPos);
                ScaleModel(modelName, cubeSize);
                MakeMeshAABB(modelName, true);
                UseManualColors(modelName, true);
                ChangeColor(modelName, 0.90f, 0.1f, 0.1f);
                TurnWireframeModeOn(modelName);

                //-------------------------Create AABB instances to store vertices and triangles---------------------------
                
                cAABB* newBoundingBox = new cAABB();

                newBoundingBox->meshModelName = modelName;

                newBoundingBox->minXYZ.x = xPos - cubeSize / 2;
                newBoundingBox->minXYZ.y = yPos - cubeSize / 2;
                newBoundingBox->minXYZ.z = zPos - cubeSize / 2;

                newBoundingBox->maxXYZ.x = xPos + cubeSize / 2;
                newBoundingBox->maxXYZ.y = yPos + cubeSize / 2;
                newBoundingBox->maxXYZ.z = zPos + cubeSize / 2;

                AABBList.push_back(newBoundingBox);
            }
        }
    }

    bAABBGenerated = true;
}

void cControlGameEngine::AddVerticesToAABB(std::string modelName)
{
    //-------------------------------Checking Vertices in each AABB-------------------------------------------------------

    cMesh* currentMesh = g_pFindMeshByFriendlyName(modelName);
    sModelDrawInfo* currentMeshModel = g_pFindModelInfoByFriendlyName(modelName);

    if (!currentMesh->bIsAABBMesh)
    {
        for (unsigned int index = 0; index < currentMeshModel->numberOfVertices; index++)
        {
            //---------------Calculate vertex position-----------------------------

            glm::vec3 vertex;

            vertex.x = currentMeshModel->pVertices[index].x;
            vertex.y = currentMeshModel->pVertices[index].y;
            vertex.z = currentMeshModel->pVertices[index].z;

            //----------------Convert to world space------------------------------

            glm::mat4 matModel = glm::mat4(1.0f);

            glm::mat4 matTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(currentMesh->drawPosition.x, currentMesh->drawPosition.y, currentMesh->drawPosition.z));

            glm::mat4 matRotate = glm::mat4_cast(currentMesh->get_qOrientation());

            glm::mat4 matScale = glm::scale(glm::mat4(1.0f), glm::vec3(currentMesh->drawScale.x, currentMesh->drawScale.y, currentMesh->drawScale.z));

            matModel = matModel * matTranslate;

            matModel = matModel * matRotate;

            matModel = matModel * matScale;

            glm::vec4 vertexInWorldSpace;

            vertexInWorldSpace = (matModel * glm::vec4(vertex, 1.0f));

            glm::vec3 newVertices = glm::vec3(vertexInWorldSpace.x, vertexInWorldSpace.y, vertexInWorldSpace.z);
                
            for (cAABB* aabbMesh : AABBList)
            {
                if (aabbMesh->IsInsideAABB(newVertices))
                {
                    aabbMesh->verticesInsideAABBList.push_back(newVertices);
                    break;
                }
            }
        }
    }

    std::cout << "Total vertices : " << currentMeshModel->numberOfVertices<<std::endl;
    int sumVertices = 0;

    for (int i = 0; i < AABBList.size(); i++)
    {
        std::cout << "AABB[" << i << "] has " << AABBList[i]->verticesInsideAABBList.size() << " vertices inside it." << std::endl;
        sumVertices += AABBList[i]->verticesInsideAABBList.size();
    }

    std::cout << "Vertices inside AABBs : " << sumVertices<<std::endl;
}

void cControlGameEngine::AddTrianglesToAABB(std::string modelName)
{
    //-------------------------------Checking Vertices in each AABB-------------------------------------------------------

    cMesh* currentMesh = g_pFindMeshByFriendlyName(modelName);
    sModelDrawInfo* currentMeshModel = g_pFindModelInfoByFriendlyName(modelName);

    if (!currentMesh->bIsAABBMesh)
    {
        for (unsigned int index = 0; index < currentMeshModel->numberOfIndices; index+= 3)
        {
            glm::vec3 vertices[3];

            unsigned int triangleIndex_0 = currentMeshModel->pIndices[index];
            unsigned int triangleIndex_1 = currentMeshModel->pIndices[index + 1];
            unsigned int triangleIndex_2 = currentMeshModel->pIndices[index + 2];

            vertices[0].x = currentMeshModel->pVertices[triangleIndex_0].x;
            vertices[0].y = currentMeshModel->pVertices[triangleIndex_0].y;
            vertices[0].z = currentMeshModel->pVertices[triangleIndex_0].z;
                         
            vertices[1].x = currentMeshModel->pVertices[triangleIndex_1].x;
            vertices[1].y = currentMeshModel->pVertices[triangleIndex_1].y;
            vertices[1].z = currentMeshModel->pVertices[triangleIndex_1].z;
                         
            vertices[2].x = currentMeshModel->pVertices[triangleIndex_2].x;
            vertices[2].y = currentMeshModel->pVertices[triangleIndex_2].y;
            vertices[2].z = currentMeshModel->pVertices[triangleIndex_2].z;

            //----------------Convert to world space------------------------------

            glm::mat4 matModel = glm::mat4(1.0f);

            glm::mat4 matTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(currentMesh->drawPosition.x, currentMesh->drawPosition.y, currentMesh->drawPosition.z));

            glm::mat4 matRotate = glm::mat4_cast(currentMesh->get_qOrientation());

            glm::mat4 matScale = glm::scale(glm::mat4(1.0f), glm::vec3(currentMesh->drawScale.x, currentMesh->drawScale.y, currentMesh->drawScale.z));

            matModel = matModel * matTranslate;

            matModel = matModel * matRotate;

            matModel = matModel * matScale;

            glm::vec4 vertexInWorldSpace[3];

            vertexInWorldSpace[0] = (matModel * glm::vec4(vertices[0], 1.0f));
            vertexInWorldSpace[1] = (matModel * glm::vec4(vertices[1], 1.0f));
            vertexInWorldSpace[2] = (matModel * glm::vec4(vertices[2], 1.0f));

            sAABBTriangle newTriangle;

            newTriangle.vertex1 = glm::vec3(vertexInWorldSpace[0].x, vertexInWorldSpace[0].y, vertexInWorldSpace[0].z);
            newTriangle.vertex2 = glm::vec3(vertexInWorldSpace[1].x, vertexInWorldSpace[1].y, vertexInWorldSpace[1].z);
            newTriangle.vertex3 = glm::vec3(vertexInWorldSpace[2].x, vertexInWorldSpace[2].y, vertexInWorldSpace[2].z);

            for (cAABB* aabbMesh : AABBList)
            {
                if (aabbMesh->IsInsideAABB(newTriangle.vertex1) || aabbMesh->IsInsideAABB(newTriangle.vertex2) || aabbMesh->IsInsideAABB(newTriangle.vertex3))
                {
                    aabbMesh->trianglesInsideAABBList.push_back(newTriangle);
                    break;
                }
            }
        }

    }

    std::cout << "Total triangles : " << currentMeshModel->numberOfTriangles << std::endl;
    int sumTriangles = 0;

    for (int i = 0; i < AABBList.size(); i++)
    {
        std::cout << "AABB[" << i << "] has " << AABBList[i]->trianglesInsideAABBList.size() << " traingles inside it." << std::endl;
        sumTriangles += AABBList[i]->trianglesInsideAABBList.size();
    }

    std::cout << "Triangles inside AABBs : " << sumTriangles << std::endl;
}

//-------------------------------------SOFT BODY CONTROLS---------------------------------------------------------------

void cControlGameEngine::CreateVerletSoftBody(std::string modelName, glm::vec3 softbodyAcceleration, bool createDebugSpheres, float scaleVal, glm::vec3 sphereRGBColor)
{
    cSoftBody* newSoftBody = new cSoftBody();

    sModelDrawInfo softBodyObjectDrawingInfo;

    cMesh* meshFileNameNeeded = g_pFindMeshByFriendlyName(modelName);

    meshFileNameNeeded->bIsSoftBody;

    newSoftBody->acceleration = softbodyAcceleration;

    if (mVAOManager->FindDrawInfoByModelName(meshFileNameNeeded->friendlyName, softBodyObjectDrawingInfo))
    {
        glm::mat4 matTransform = glm::translate(glm::mat4(1.0f), meshFileNameNeeded->drawPosition);
        matTransform = glm::scale(matTransform, meshFileNameNeeded->drawScale);

        if (newSoftBody->CreateSoftBody(softBodyObjectDrawingInfo, matTransform))
        {
            std::cout << std::endl;
            std::cout << "SUCCESS : CREATED SOFT BODY FOR THE MODEL - [" << meshFileNameNeeded->meshName << "]" << std::endl;
        }
        else
        {
            std::cout << std::endl;
            std::cout << "ERROR : UNABLE TO CREATE SOFT BODY FOR THE MODEL - [" << meshFileNameNeeded->meshName << "]" << std::endl;

            return;
        }
    }
    else
    {
        std::cout << std::endl;
        std::cout << "ERROR : UNABLE TO FIND THE MODEL - [" << meshFileNameNeeded->meshName << "]" << std::endl;
    }

    if (createDebugSpheres)
    {
        newSoftBody->bEnableDebugSphere = true;
        std::string sphereModelName;

        for (unsigned int index = 0; index < newSoftBody->nodesList.size(); index++)
        {
            sphereModelName = "DebugSphere_" + std::to_string(index);

            LoadModelsInto3DSpace("Sphere_1_unit_Radius_uv.ply", sphereModelName, newSoftBody->nodesList[index]->currentPosition.x, 
                newSoftBody->nodesList[index]->currentPosition.y, newSoftBody->nodesList[index]->currentPosition.z);

            ScaleModel(sphereModelName, scaleVal);
            
            TurnWireframeModeOn(sphereModelName);

            UseManualColors(sphereModelName, true);

            ChangeColor(sphereModelName, sphereRGBColor.x, sphereRGBColor.y, sphereRGBColor.z);

            newSoftBody->debugSpheresMeshNameList.push_back(sphereModelName);
        }
    }
    
    newSoftBody->bLockFlagPos = true;

    SoftBodyList.push_back(newSoftBody);
}

void cControlGameEngine::TearDownFlag()
{
    for (unsigned int index = 0; index < SoftBodyList.size(); index++)
    {
        SoftBodyList[index]->bActivateTearDown = true;
        SoftBodyList[index]->bLockFlagPos = false;

        for (int count = 1; count <= SoftBodyList[index]->WIND_SPHERE_COUNT; count++)
        {
            cSoftBody::sGustOfWind* newWindSphere = new cSoftBody::sGustOfWind();

            newWindSphere->radius = 2.f;
            newWindSphere->windPos = glm::vec3(-7.0f + count * 0.5, -20.0f, 0.0f);
            newWindSphere->destinationPos = glm::vec3(-7.0f + count * 0.5, 15.0f, 0.0f);

            SoftBodyList[index]->sphereWindList.push_back(newWindSphere);
        }
    }
}

void cControlGameEngine::MakeWind(int num)
{
    if (num == 1)
    {
        for (unsigned int index = 0; index < SoftBodyList.size(); index++)
        {
            for (int count = 1; count <= SoftBodyList[index]->WIND_SPHERE_COUNT; count++)
            {
                cSoftBody::sGustOfWind* newWindSphere = new cSoftBody::sGustOfWind();

                newWindSphere->radius = 2.f;
                newWindSphere->windPos = glm::vec3(-7.0f + count * 0.5, -20.0f, 0.0f);
                newWindSphere->destinationPos = glm::vec3(-7.0f + count * 0.5, 15.0f, 0.0f);

                SoftBodyList[index]->sphereWindList.push_back(newWindSphere);
            }
        }
    }

    else if (num == 2)
    {
        for (unsigned int index = 0; index < SoftBodyList.size(); index++)
        {
            for (int count = 1; count <= SoftBodyList[index]->WIND_SPHERE_COUNT; count++)
            {
                cSoftBody::sGustOfWind* newWindSphere = new cSoftBody::sGustOfWind();

                newWindSphere->radius = 2.f;
                newWindSphere->windPos = glm::vec3(-7.0f + count * 0.5, 15.0f, 0.0f);
                newWindSphere->destinationPos = glm::vec3(-7.0f + count * 0.5, -20.0f, 0.0f);

                SoftBodyList[index]->sphereWindList.push_back(newWindSphere);
            }
        }
    }


    else if (num == 3)
    {
        for (unsigned int index = 0; index < SoftBodyList.size(); index++)
        {
            for (int count = 1; count <= SoftBodyList[index]->WIND_SPHERE_COUNT; count++)
            {
                cSoftBody::sGustOfWind* newWindSphere = new cSoftBody::sGustOfWind();

                newWindSphere->radius = 2.f;
                newWindSphere->windPos = glm::vec3(0.0f, 0.0f, -10.0f + count * 0.5);
                newWindSphere->destinationPos = glm::vec3(0.0f , 0.0f, 10.0f + count * 0.5);

                SoftBodyList[index]->sphereWindList.push_back(newWindSphere);
            }
        }
    }

    else if (num == 4)
    {
        for (unsigned int index = 0; index < SoftBodyList.size(); index++)
        {
            for (int count = 1; count <= SoftBodyList[index]->WIND_SPHERE_COUNT; count++)
            {
                cSoftBody::sGustOfWind* newWindSphere = new cSoftBody::sGustOfWind();

                newWindSphere->radius = 2.f;
                newWindSphere->windPos = glm::vec3(0.0f, 0.0f, 10.0f + count * 0.5);
                newWindSphere->destinationPos = glm::vec3(0.0f, 0.0f, -10.0f + count * 0.5);

                SoftBodyList[index]->sphereWindList.push_back(newWindSphere);
            }
        }
    }
}

void cControlGameEngine::UpdateSoftBodyTransformation()
{
    for (unsigned int index = 0; index < SoftBodyList.size(); index++)
    {
        SoftBodyList[index]->VerletUpdate(deltaTime);
        SoftBodyList[index]->ApplyCollision(deltaTime);
        SoftBodyList[index]->SatisfyConstraints();
        SoftBodyList[index]->UpdateVertexPositions();
        SoftBodyList[index]->UpdateNormals();

        mVAOManager->UpdateVertexBuffers(SoftBodyList[index]->mModelVertexInfo.friendlyName, SoftBodyList[index]->mModelVertexInfo, shaderProgramID);

        if (SoftBodyList[index]->bEnableDebugSphere)
        {
            for (unsigned int vertexIndex = 0; vertexIndex < SoftBodyList[index]->nodesList.size(); vertexIndex++)
                MoveModel(SoftBodyList[index]->debugSpheresMeshNameList[vertexIndex], SoftBodyList[index]->nodesList[vertexIndex]->currentPosition.x,
                    SoftBodyList[index]->nodesList[vertexIndex]->currentPosition.y, SoftBodyList[index]->nodesList[vertexIndex]->currentPosition.z);
        }
    }
}

void cControlGameEngine::ToggleDebugSphereVisibility(bool turnOnVisibility)
{
    for (unsigned int index = 0; index < SoftBodyList.size(); index++)
    {
        if (turnOnVisibility)
        {
            for (unsigned int vertexIndex = 0; vertexIndex < SoftBodyList[index]->nodesList.size(); vertexIndex++)
                TurnVisibilityOn(SoftBodyList[index]->debugSpheresMeshNameList[vertexIndex]);
        }
        else
        {
            for (unsigned int vertexIndex = 0; vertexIndex < SoftBodyList[index]->nodesList.size(); vertexIndex++)
                TurnVisibilityOn(SoftBodyList[index]->debugSpheresMeshNameList[vertexIndex]);
        }

        SoftBodyList[index]->bEnableDebugSphere = turnOnVisibility;
    }
}

//--------------------------------------ENGINE CONTROLS-----------------------------------------------------------------

int cControlGameEngine::InitializeGameEngine()
{
    //-------------------------------------Shader Initialize----------------------------------------------------------------

    int result = InitializeShader();

    if (result != 0)
        return -1;

    //-------------------------------------VAO Initialize---------------------------------------------------------------------

    mVAOManager = new cVAOManager();

    mVAOManager->setBasePath("Assets/Models");

    mPhysicsManager = new cPhysics();

    mPhysicsManager->setVAOManager(mVAOManager);

    //------------------------------------Lights Initialize-----------------------------------------------------------------------

    mLightManager = new cLightManager();

    //------------------------------------Texture Initialize----------------------------------------------------------------------

    mTextureManager = new cBasicTextureManager();

    mTextureManager->SetBasePath("Assets/Textures");

    return 0;
}

void cControlGameEngine::RunGameEngine(GLFWwindow* window)
{
    float ratio;
    int width, height;

    glUseProgram(shaderProgramID);

    glfwGetFramebufferSize(window, &width, &height);

    ratio = width / static_cast<float>(height);

    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glCullFace(GL_BACK);

    //---------------------------Light Values Update----------------------------------------

    mLightManager->UpdateUniformValues(shaderProgramID);

    //---------------------------Camera Values----------------------------------------------

    GLint eyeLocation_UL = glGetUniformLocation(shaderProgramID, "eyeLocation");
    glUniform4f(eyeLocation_UL,
        cameraEye.x, cameraEye.y, cameraEye.z, 1.0f);

    glm::mat4 matProjection = glm::perspective(0.6f, ratio, 0.1f, 10'000.0f);

    glm::mat4 matView = glm::lookAt(cameraEye, cameraEye + cameraTarget, upVector);

    GLint matProjection_UL = glGetUniformLocation(shaderProgramID, "matProjection");
    glUniformMatrix4fv(matProjection_UL, 1, GL_FALSE, glm::value_ptr(matProjection));

    GLint matView_UL = glGetUniformLocation(shaderProgramID, "matView");
    glUniformMatrix4fv(matView_UL, 1, GL_FALSE, glm::value_ptr(matView));

    //----------------------------Draw all the objects-------------------------------------------

    cMesh* skyBox = NULL;

    // Sorts the TotalMeshList based on distance between camera position and mesh object positions 
    if(bTransparencyMeshAvailable)
        SortMeshesBasedOnTransparency();

    for (unsigned int index = 0; index != TotalMeshList.size(); index++)
    {
        cMesh* currentMesh = TotalMeshList[index];

        if (currentMesh->bIsSkyBox && !bTransparencyMeshAvailable)
            skyBox = currentMesh;
        else
        {
            if (currentMesh->bIsVisible)
            {
                if (!currentMesh->bIsSkyBox)
                {
                    glm::mat4 matModel = glm::mat4(1.0f);

                    DrawObject(currentMesh, matModel, shaderProgramID);
                }
                else
                    DrawSkyBox(currentMesh);
            }
        }

        // This is to draw skyBox at the last if there are no transparent objects in the 3D world
        if (index == TotalMeshList.size() - 1 && skyBox != NULL)
            DrawSkyBox(skyBox);
    }

    //----------------------------Title Screen Values---------------------------------------------

    std::stringstream ssTitle;

    //-----------------Light values displayed - Commented------------------------

   /* int lightId = lightListIndex;

    glm::vec3 lightPosition = GetLightPosition(lightId);
    float lightLinearAtten = GetLightLinearAttenuation(lightId);
    float lightQuadraticAtten = GetLightQuadraticAttenuation(lightId);
    glm::vec3 lightDirection = GetLightDirection(lightId);
    float lightType = GetLightType(lightId);
    float lightInnerAngle = GetLightInnerAngle(lightId);
    float lightOuterAngle = GetLightOuterAngle(lightId);

    ssTitle << "Light Id : "
        << lightId << " | Light Position : ("
        << lightPosition.x << ", "
        << lightPosition.y << ", "
        << lightPosition.z << ") | Direction : ("
        << lightDirection.x << ", "
        << lightDirection.y << ", "
        << lightDirection.z << ") |  Linear Atten : "
        << lightLinearAtten << " | Quadratic Atten : "
        << lightQuadraticAtten << " | Type : "
        << lightType << " | Inner Angle : "
        << lightInnerAngle << " | Outer Angle : "
        << lightOuterAngle;*/

        //----------------Cam and Model values displayed-----------------------------

    cMesh* meshObj = GetCurrentModelSelected();

    ssTitle << "Camera Eye(x, y, z) : ("
        << cameraEye.x << ", "
        << cameraEye.y << ", "
        << cameraEye.z << ") | "
        << "Camera Target(x,y,z): ("
        << cameraTarget.x << ", "
        << cameraTarget.y << ", "
        << cameraTarget.z << ") | Yaw/Pitch : ("
        << yaw << ", " << pitch << ") | ModelName : "
        << meshObj->friendlyName << " | ModelPos : ("
        << meshObj->drawPosition.x << ", "
        << meshObj->drawPosition.y << ", "
        << meshObj->drawPosition.z << ") | ModelScaleVal : "
        << meshObj->drawScale.x << " | ModelRotateDeg : ("
        << meshObj->drawOrientation.x << ", "
        << meshObj->drawOrientation.y << ", "
        << meshObj->drawOrientation.z << ")";

    std::string theTitle = ssTitle.str();

    glfwSwapBuffers(window);

    glfwPollEvents();

    glfwSetWindowTitle(window, theTitle.c_str());
}


