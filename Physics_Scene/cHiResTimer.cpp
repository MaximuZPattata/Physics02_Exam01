#include "pch.h"
#include "cHiResTimer.h"

cHiResTimer::cHiResTimer(unsigned int numberOfSamples)
{
    this->mLastTime = glfwGetTime();
    this->mMaxDeltaTime = cHiResTimer::MAX_DELTA_TIME_DEFAULT;

    for (int count = 0; count != numberOfSamples; count++)
        this->deltaTimeList.push_back(cHiResTimer::MAX_DELTA_TIME_DEFAULT);
}

const double cHiResTimer::MAX_DELTA_TIME_DEFAULT = 1.0f / 60.0f;

void cHiResTimer::setMaxDelaTime(double newMaxDeltaTime)
{
    this->mMaxDeltaTime = newMaxDeltaTime;
    return;
}

double cHiResTimer::getFrameTime(void)
{
    //------------New Method---------------------

    double currentTime = glfwGetTime();
    double totalDeltaTime = 0.0;
    double averageDeltaTime = 0.0;
    double deltaTime = currentTime - mLastTime;

    if (deltaTime > cHiResTimer::MAX_DELTA_TIME_DEFAULT)
        deltaTime = cHiResTimer::MAX_DELTA_TIME_DEFAULT;

    this->mLastTime = currentTime;

    this->deltaTimeList[this->mNextBufferIndex] = deltaTime;
    this->mNextBufferIndex++;

    if (this->mNextBufferIndex >= (unsigned int)this->deltaTimeList.size())
        this->mNextBufferIndex = 0;

    for (unsigned int index = 0; index != this->deltaTimeList.size(); index++)
        totalDeltaTime += this->deltaTimeList[index];

    averageDeltaTime = totalDeltaTime / (double)this->deltaTimeList.size();

    return averageDeltaTime;

    //-------------Old Method--------------------

    /*double currentTime = glfwGetTime();

    double deltaTime = currentTime - mLastTime;

    const double LARGEST_DELTA_TIME = 1.f / 30.0f;

    if (deltaTime > LARGEST_DELTA_TIME)
        deltaTime = LARGEST_DELTA_TIME;

    mLastTime = currentTime;

    return deltaTime;*/
}