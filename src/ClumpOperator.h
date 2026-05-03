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

    // curl
    float curlRadius;
    float curlFrequency;
    float curlRandomFrequency;
    float curlStart;

    float noiseAmplitude;
    float noiseFrequency;
};

#endif
