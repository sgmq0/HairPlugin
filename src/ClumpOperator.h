#ifndef CLUMP_OPERATOR_H
#define CLUMP_OPERATOR_H

#include <UT/UT_Vector3.h>

// Full parameter struct for clumping with all operators
struct ClumpParams
{
    float scaleFactor;

    float clumpRadius;
    float clumpTightness;
    int clumpCount;

    float twistAmount;
    float twistFrequency;

    float bendMagnitude;
    UT_Vector3 bendDirection;

    float noiseAmplitude;
    float noiseFrequency;
};

#endif
