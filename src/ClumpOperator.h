#ifndef CLUMP_OPERATOR_H
#define CLUMP_OPERATOR_H

#include "StrandSet.h"
#include "GuideSet.h"
#include <vector>

// Struct to hold all clump operation parameters
struct ClumpOperatorParams
{
    // Clump operation
    float clumpRadius;      // Size of clumping zones (0.1 - 10.0)
    float clumpTightness;   // Strength of clumping attraction (0.0 - 1.0)
    int clumpCount;         // Number of clumps per guide (1 - 100)

    // Twist operation
    float twistAmount;      // Rotation amount in degrees per unit length (0.0 - 360.0)
    float twistFrequency;   // Frequency of twist oscillation (0.0 - 10.0)

    // Noise operation
    float noiseFrequency;   // Frequency of Perlin noise (0.1 - 10.0)
    float noiseAmplitude;   // Amplitude of noise displacement (0.0 - 1.0)
    float noiseScale;       // Scale of noise pattern (0.1 - 10.0)

    // Bend operation
    UT_Vector3 bendDirection;   // Global deformation direction
    float bendMagnitude;        // Strength of bending (0.0 - 1.0)

    ClumpOperatorParams()
        : clumpRadius(1.0f), clumpTightness(0.5f), clumpCount(20),
        twistAmount(0.0f), twistFrequency(1.0f),
        noiseFrequency(1.0f), noiseAmplitude(0.0f), noiseScale(1.0f),
        bendDirection(0, 0, 0), bendMagnitude(0.0f)
    {
    }
};

class ClumpOperator
{
public:
    // Main function: Generate clumped strands from guides with all transformations
    static StrandSet applyClumpOperations(
        const GuideSet& guides,
        const ClumpOperatorParams& params
    );

private:
    // Individual transformation operations

    // Clump: Group nearby strands via attraction forces
    static std::vector<Strand> clumpAroundGuide(
        const Strand& guide,
        float clumpRadius,
        float clumpTightness,
        int clumpCount);

    // Create a single clumped strand offset from guide
    static Strand createClumpedStrand(
        const Strand& guide,
        float offsetRadius,
        float tightness);

    // Generate random offset vector with controlled spread
    static UT_Vector3 generateRandomOffset(
        float maxRadius,
        float tightness);

    // Twist: Rotate cross-section around guide centerline
    static Strand applyTwist(
        const Strand& strand,
        float twistAmount,
        float twistFrequency);

    // Get perpendicular vector (for twist rotation)
    static UT_Vector3 getPerpendicular(const UT_Vector3& v);

    // Rotate vector around axis
    static UT_Vector3 rotateAroundAxis(
        const UT_Vector3& point,
        const UT_Vector3& axis,
        const UT_Vector3& center,
        float angleRadians);

    // Noise: Add Perlin noise-based variation
    static Strand applyNoise(
        const Strand& strand,
        float noiseFrequency,
        float noiseAmplitude,
        float noiseScale);

    // Simple Perlin noise implementation (or wrapper)
    static float perlinNoise3D(float x, float y, float z);

    // Bend: Apply global directional deformation
    static Strand applyBend(
        const Strand& strand,
        const UT_Vector3& bendDirection,
        float bendMagnitude);

    // Helper: Apply all transformations in sequence
    static Strand applyAllTransformations(
        const Strand& strand,
        const ClumpOperatorParams& params);
};

#endif
