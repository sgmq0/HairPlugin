#include "ClumpOperator.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

StrandSet ClumpOperator::clumpFromGuides(
    const GuideSet& guides,
    const ClumpParams& params)
{
    StrandSet result;

    if (guides.getGuideCount() == 0)
        return result;

    // For each guide, generate clumped strands
    for (int g = 0; g < guides.getGuideCount(); ++g) {
        const Strand& guide = guides.getGuide(g);

        if (guide.positions.size() < 2) {
            continue;
        }

        std::vector<Strand> clumpedStrands = clumpAroundGuide(guide, params);

        for (const Strand& strand : clumpedStrands) {
            result.addStrand(strand);
        }
    }

    return result;
}

std::vector<Strand> ClumpOperator::clumpAroundGuide(
    const Strand& guide,
    const ClumpParams& params)
{
    std::vector<Strand> result;

    if (guide.positions.size() < 2)
        return result;

    // Generate clumpCount strands around this guide
    for (int i = 0; i < params.clumpCount; ++i) {
        Strand clumped = createClumpedStrand(
            guide,
            params.clumpRadius,
            params.clumpTightness,
            params);

        if (clumped.positions.size() >= 2) {
            result.push_back(clumped);
        }
    }

    return result;
}

Strand ClumpOperator::createClumpedStrand(
    const Strand& guide,
    float offsetRadius,
    float tightness,
    const ClumpParams& params)
{
    Strand result;

    if (guide.positions.size() < 2) {
        return result;
    }

    // Generate random offset
    UT_Vector3 offset = generateRandomOffset(offsetRadius, tightness);

    // Create positions by offsetting guide
    result.positions.reserve(guide.positions.size());
    result.radius.reserve(guide.positions.size());

    for (size_t i = 0; i < guide.positions.size(); ++i) {
        UT_Vector3 pos = guide.positions[i];
        // Apply full offset along entire strand (no fade-out)
        pos += offset;
        result.positions.push_back(pos);
        result.radius.push_back(guide.radius.empty() ? 0.1f : guide.radius[i]);
    }

    // Apply transformations
    if (params.twistAmount > 1e-6f) {
        result = applyTwist(result, params.twistAmount, params.twistFrequency);
    }

    if (params.bendMagnitude > 1e-6f) {
        result = applyBend(result, params.bendDirection, params.bendMagnitude);
    }

    if (params.noiseAmplitude > 1e-6f) {
        result = applyNoise(result, params.noiseAmplitude, params.noiseFrequency);
    }

    // Recompute arc length
    result.arcLength = 0.0f;
    for (size_t i = 1; i < result.positions.size(); ++i) {
        UT_Vector3 diff = result.positions[i] - result.positions[i - 1];
        result.arcLength += diff.length();
    }

    result.clusterID = guide.clusterID;
    return result;
}

UT_Vector3 ClumpOperator::generateRandomOffset(
    float maxRadius,
    float tightness)
{
    // maxRadius controls the size of clumping zone
    // tightness controls concentration (1.0 = spread out, 0.0 = concentrated)
    float radius = maxRadius;  // Always use full radius, tightness affects distribution

    float x = (float)rand() / RAND_MAX * 2.0f - 1.0f;
    float y = (float)rand() / RAND_MAX * 2.0f - 1.0f;
    float z = (float)rand() / RAND_MAX * 2.0f - 1.0f;

    UT_Vector3 offset;
    offset.assign(x, y, z);
    offset.normalize();

    // Tightness affects the distribution - higher tightness = more spread out
    // Lower tightness = more concentrated
    if (tightness < 0.5f) {
        // Concentrated distribution - raise to power to concentrate towards center
        float power = 2.0f - (tightness * 4.0f);  // Power from 2.0 to 0.0
        radius *= std::pow((float)rand() / RAND_MAX, power);
    }
    else {
        // Spread out distribution - uniform
        radius *= (float)rand() / RAND_MAX;
    }

    offset *= radius;

    return offset;
}

Strand ClumpOperator::applyTwist(
    const Strand& strand,
    float twistAmount,
    float twistFrequency)
{
    Strand result = strand;

    if (strand.positions.size() < 2) {
        return result;
    }

    // Get strand direction (root to tip)
    UT_Vector3 rootToTip = strand.positions.back() - strand.positions.front();
    float strandLength = rootToTip.length();

    if (strandLength < 1e-6f) {
        return result;
    }

    rootToTip /= strandLength;

    // Perpendicular basis vectors
    UT_Vector3 perp1(1, 0, 0);
    if (std::abs(rootToTip.dot(perp1)) > 0.9f) {
        perp1.assign(0, 1, 0);
    }

    UT_Vector3 perp2;
    perp2 = rootToTip;
    float dotProd = perp2.dot(perp1);
    perp2 -= perp1 * dotProd;
    perp2.normalize();

    // Cross product for third basis
    UT_Vector3 perp3 = rootToTip;
    float perp1_x = perp1.x(), perp1_y = perp1.y(), perp1_z = perp1.z();
    float perp2_x = perp2.x(), perp2_y = perp2.y(), perp2_z = perp2.z();
    float root_x = rootToTip.x(), root_y = rootToTip.y(), root_z = rootToTip.z();

    perp3.assign(
        perp1_y * root_z - perp1_z * root_y,
        perp1_z * root_x - perp1_x * root_z,
        perp1_x * root_y - perp1_y * root_x
    );
    perp3.normalize();

    // Apply twist along strand
    for (size_t i = 0; i < result.positions.size(); ++i) {
        float t = (strandLength > 0) ? (float)i / (float)(result.positions.size() - 1) : 0.0f;

        // Twist angle: twistAmount is in full rotations per strand length
        // Convert to radians: twistAmount * 2π * frequency * t
        float angle = twistAmount * 6.28318f * twistFrequency * t;

        // Rotate offset perpendicular to direction
        UT_Vector3 offset = result.positions[i] - strand.positions[i];

        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Rotate around the strand direction
        float comp2 = offset.dot(perp2) * cosA - offset.dot(perp3) * sinA;
        float comp3 = offset.dot(perp2) * sinA + offset.dot(perp3) * cosA;

        UT_Vector3 rotated = perp2 * comp2 + perp3 * comp3;
        result.positions[i] = strand.positions[i] + rotated;
    }

    return result;
}

Strand ClumpOperator::applyBend(
    const Strand& strand,
    const UT_Vector3& bendDirection,
    float bendMagnitude)
{
    Strand result = strand;

    if (bendDirection.length() < 1e-6f) {
        return result;
    }

    UT_Vector3 bendDir = bendDirection;
    bendDir.normalize();

    // Apply bend as parabolic deformation
    UT_Vector3 rootToTip = strand.positions.back() - strand.positions.front();
    float strandLength = rootToTip.length();

    if (strandLength < 1e-6f) {
        return result;
    }

    for (size_t i = 0; i < result.positions.size(); ++i) {
        float t = (float)i / (float)(result.positions.size() - 1);

        // Parabolic falloff: maximum bend at middle, much stronger
        float bendAmount = bendMagnitude * strandLength * 4.0f * t * (1.0f - t);

        result.positions[i] += bendDir * bendAmount;
    }

    return result;
}

Strand ClumpOperator::applyNoise(
    const Strand& strand,
    float noiseAmplitude,
    float noiseFrequency)
{
    Strand result = strand;

    if (strand.positions.size() < 2) {
        return result;
    }

    UT_Vector3 rootToTip = strand.positions.back() - strand.positions.front();
    float strandLength = rootToTip.length();

    if (strandLength < 1e-6f) {
        return result;
    }

    // Perpendicular basis
    UT_Vector3 perp1(1, 0, 0);
    rootToTip.normalize();  // Normalize first
    if (std::abs(rootToTip.dot(perp1)) > 0.9f) {
        perp1.assign(0, 1, 0);
    }
    perp1.normalize();

    for (size_t i = 0; i < result.positions.size(); ++i) {
        float t = (float)i / (float)(result.positions.size() - 1);

        // Simple random noise in 3D
        float noise = perlinNoise3D(
            t * noiseFrequency,
            (float)i * 0.05f,
            (float)rand() / RAND_MAX);

        // Apply noise in random perpendicular direction
        UT_Vector3 randomPerp;
        randomPerp.assign(
            std::sin(t * 6.28318f),
            std::cos(t * 6.28318f),
            std::sin(t * 3.14159f));
        randomPerp.normalize();

        result.positions[i] += randomPerp * noise * noiseAmplitude * strandLength;
    }

    return result;
}

float ClumpOperator::perlinNoise3D(float x, float y, float z)
{
    // Simple Perlin-like noise using sin/cos
    // Not true Perlin, but smooth and deterministic
    float noise = 0.0f;

    // Multiple octaves
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < 3; ++i) {
        noise += amplitude * std::sin(x * frequency) *
            std::sin(y * frequency) *
            std::sin(z * frequency);
        maxValue += amplitude;

        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return noise / maxValue;  // Normalize to roughly [-1, 1]
}