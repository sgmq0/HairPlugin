#include "ClumpOperator.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

StrandSet ClumpOperator::clumpFromGuides(
    const GuideSet& guides,
    float clumpRadius,
    float clumpTightness,
    int clumpCount)
{
    StrandSet result;

    if (guides.getGuideCount() == 0)
        return result;

    // Clamp parameters to reasonable ranges
    clumpRadius = std::max(0.1f, std::min(10.0f, clumpRadius));
    clumpTightness = std::max(0.0f, std::min(1.0f, clumpTightness));
    clumpCount = std::max(1, std::min(100, clumpCount));

    // For each guide, generate clumped strands around it
    for (int g = 0; g < guides.getGuideCount(); ++g)
    {
        const Strand& guide = guides.getGuide(g);
        
        // Generate clumpCount strands around this guide
        std::vector<Strand> clumpedStrands = clumpAroundGuide(
            guide,
            clumpRadius,
            clumpTightness,
            clumpCount);

        // Add all clumped strands to result
        for (const auto& strand : clumpedStrands)
        {
            result.addStrand(strand);
        }
    }

    return result;
}

std::vector<Strand> ClumpOperator::clumpAroundGuide(
    const Strand& guide,
    float clumpRadius,
    float clumpTightness,
    int clumpCount)
{
    std::vector<Strand> clumpedStrands;

    if (guide.positions.size() < 2)
        return clumpedStrands;

    // Generate clumpCount strands offset from the guide
    for (int c = 0; c < clumpCount; ++c)
    {
        Strand clumpedStrand = createClumpedStrand(
            guide,
            clumpRadius,
            clumpTightness);

        clumpedStrands.push_back(clumpedStrand);
    }

    return clumpedStrands;
}

Strand ClumpOperator::createClumpedStrand(
    const Strand& guide,
    float offsetRadius,
    float tightness)
{
    Strand clumpedStrand;
    clumpedStrand.radius = guide.radius;
    clumpedStrand.clusterID = guide.clusterID;

    // Generate random offset perpendicular to guide
    UT_Vector3 offset = generateRandomOffset(offsetRadius, tightness);

    // Create positions by offsetting guide positions
    clumpedStrand.positions.reserve(guide.positions.size());

    for (size_t i = 0; i < guide.positions.size(); ++i)
    {
        // Add base offset
        UT_Vector3 pos = guide.positions[i] + offset;

        // Add some variation along the strand (tightness controls this)
        if (i > 0 && i < guide.positions.size() - 1 && tightness < 1.0f)
        {
            // Add small perpendicular oscillation
            float variation = (1.0f - tightness) * offsetRadius * 0.1f;
            UT_Vector3 randomVariation = generateRandomOffset(variation, 0.5f);
            pos += randomVariation;
        }

        clumpedStrand.positions.push_back(pos);
    }

    // Compute arc length
    clumpedStrand.arcLength = 0.0f;
    for (size_t i = 1; i < clumpedStrand.positions.size(); ++i)
    {
        float segLen = (clumpedStrand.positions[i] - clumpedStrand.positions[i - 1]).length();
        clumpedStrand.arcLength += segLen;
    }

    return clumpedStrand;
}

UT_Vector3 ClumpOperator::generateRandomOffset(
    float maxRadius,
    float tightness)
{
    // Generate random vector with controlled distribution
    // tightness = 1.0 → uniform distribution in sphere
    // tightness = 0.0 → gaussian distribution (concentrated at center)

    // Random angles
    float theta = 2.0f * 3.14159f * ((float)std::rand() / RAND_MAX);
    float phi = 3.14159f * ((float)std::rand() / RAND_MAX);

    // Random radius (with tightness control)
    float r;
    if (tightness > 0.5f)
    {
        // Uniform distribution in sphere
        float u = (float)std::rand() / RAND_MAX;
        r = maxRadius * std::cbrt(u);
    }
    else
    {
        // Gaussian-like distribution (concentrated)
        float u = (float)std::rand() / RAND_MAX;
        float v = (float)std::rand() / RAND_MAX;
        // Box-Muller transform for gaussian
        float gaussian = std::sqrt(-2.0f * std::log(u + 1e-6f)) * std::cos(2.0f * 3.14159f * v);
        r = maxRadius * gaussian * (1.0f - tightness);
        r = std::abs(r);
    }

    // Convert spherical to cartesian
    float x = r * std::sin(phi) * std::cos(theta);
    float y = r * std::sin(phi) * std::sin(theta);
    float z = r * std::cos(phi);

    return UT_Vector3(x, y, z);
}
