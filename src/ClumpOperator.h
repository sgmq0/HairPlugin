#ifndef CLUMP_OPERATOR_H
#define CLUMP_OPERATOR_H

#include <UT/UT_Vector3.h>

// Full parameter struct for clumping with all operators
struct ClumpParams
{
    // scale
    float scaleFactor;

    // clump
    float clumpProfile;

    // bend
    float bendAngle;
    float bendStart;

    float twistAmount;
    float twistFrequency;

    float noiseAmplitude;
    float noiseFrequency;
};

#endif
