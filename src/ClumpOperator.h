#ifndef CLUMP_OPERATOR_H
#define CLUMP_OPERATOR_H

#include "StrandSet.h"
#include "GuideSet.h"
#include <vector>

// Full parameter struct for clumping with all operators
struct ClumpParams
{
    float clumpRadius = 1.0f;
    float clumpTightness = 0.5f;
    int clumpCount = 20;
    
    // Twist operator
    float twistAmount = 0.0f;      // Degrees per unit length
    float twistFrequency = 1.0f;   // Oscillation frequency
    
    // Bend operator
    float bendMagnitude = 0.0f;    // Strength of bending
    UT_Vector3 bendDirection;      // Global bend direction
    
    // Noise operator
    float noiseAmplitude = 0.0f;   // Strength of noise
    float noiseFrequency = 1.0f;   // Frequency of noise
    
    ClumpParams() : bendDirection(0, 0, 1) {}
};

class ClumpOperator
{
public:
    // Main: Generate clumped strands from guides
    static StrandSet clumpFromGuides(
        const GuideSet& guides,
        const ClumpParams& params);

private:
    static std::vector<Strand> clumpAroundGuide(
        const Strand& guide,
        const ClumpParams& params);

    static Strand createClumpedStrand(
        const Strand& guide,
        float offsetRadius,
        float tightness,
        const ClumpParams& params);

    static UT_Vector3 generateRandomOffset(
        float maxRadius,
        float tightness);

    // Transformation operators
    static Strand applyTwist(
        const Strand& strand,
        float twistAmount,
        float twistFrequency);

    static Strand applyBend(
        const Strand& strand,
        const UT_Vector3& bendDirection,
        float bendMagnitude);

    static Strand applyNoise(
        const Strand& strand,
        float noiseAmplitude,
        float noiseFrequency);

    static float perlinNoise3D(float x, float y, float z);
};

#endif
