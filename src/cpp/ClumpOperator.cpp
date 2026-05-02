#include "ClumpOperator.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Simple hash-based 3D Perlin noise
// In production, use a proper Perlin noise library
static float hash(float n)
{
    return std::fmod(std::sin(n) * 43758.5453f, 1.0f);
}

static float interpolate(float a, float b, float t)
{
    float ft = t * 3.14159265f;
    float f = (1.0f - std::cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

float ClumpOperator::perlinNoise3D(float x, float y, float z)
{
    // Simple hash-based noise (not true Perlin, but good approximation)
    float n = std::sin(x * 12.9898f + y * 78.233f + z * 45.164f) * 43758.5453f;
    return std::fmod(n, 1.0f);
}

StrandSet ClumpOperator::applyClumpOperations(
    const GuideSet& guides,
    const ClumpOperatorParams& params)
{
    StrandSet result;

    if (guides.getGuideCount() == 0)
        return result;

    // For each guide, generate clumped strands and apply transformations
    for (int g = 0; g < guides.getGuideCount(); ++g)
    {
        const Strand& guide = guides.getGuide(g);
        
        // Step 1: Apply clumping to generate offset strands
        std::vector<Strand> clumpedStrands = clumpAroundGuide(
            guide,
            params.clumpRadius,
            params.clumpTightness,
            params.clumpCount);

        // Step 2-5: Apply twist, noise, and bend to each clumped strand
        for (auto& strand : clumpedStrands)
        {
            strand = applyAllTransformations(strand, params);
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

    // Clamp parameters
    clumpRadius = std::max(0.1f, std::min(10.0f, clumpRadius));
    clumpTightness = std::max(0.0f, std::min(1.0f, clumpTightness));
    clumpCount = std::max(1, std::min(100, clumpCount));

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

UT_Vector3 ClumpOperator::getPerpendicular(const UT_Vector3& v)
{
    // Get a vector perpendicular to v
    UT_Vector3 perp;
    if (std::abs(v.x()) < 0.9f)
    {
        perp = UT_Vector3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        perp = UT_Vector3(0.0f, 1.0f, 0.0f);
    }

    // Compute cross product: v × perp
    UT_Vector3 result;
    result.x() = v.y() * perp.z() - v.z() * perp.y();
    result.y() = v.z() * perp.x() - v.x() * perp.z();
    result.z() = v.x() * perp.y() - v.y() * perp.x();

    return result;
}

UT_Vector3 ClumpOperator::rotateAroundAxis(
    const UT_Vector3& point,
    const UT_Vector3& axis,
    const UT_Vector3& center,
    float angleRadians)
{
    // Rodrigues' rotation formula
    // Rotate point around axis passing through center

    UT_Vector3 v = point - center;  // Vector from center to point
    UT_Vector3 k = axis;             // Rotation axis (make a copy)
    k.normalize();

    float cosA = std::cos(angleRadians);
    float sinA = std::sin(angleRadians);

    // Rodrigues formula: v_rot = v*cos(θ) + (k×v)*sin(θ) + k*(k·v)*(1-cos(θ))

    // Compute k × v
    UT_Vector3 kCrossV;
    kCrossV.x() = k.y() * v.z() - k.z() * v.y();
    kCrossV.y() = k.z() * v.x() - k.x() * v.z();
    kCrossV.z() = k.x() * v.y() - k.y() * v.x();

    // Compute k · v
    float kDotV = k.x() * v.x() + k.y() * v.y() + k.z() * v.z();

    // Combine: v_rot = v*cos(θ) + (k×v)*sin(θ) + k*(k·v)*(1-cos(θ))
    UT_Vector3 rotated;
    rotated.x() = v.x() * cosA + kCrossV.x() * sinA + k.x() * kDotV * (1.0f - cosA);
    rotated.y() = v.y() * cosA + kCrossV.y() * sinA + k.y() * kDotV * (1.0f - cosA);
    rotated.z() = v.z() * cosA + kCrossV.z() * sinA + k.z() * kDotV * (1.0f - cosA);

    return center + rotated;
}

Strand ClumpOperator::applyTwist(
    const Strand& strand,
    float twistAmount,
    float twistFrequency)
{
    if (twistAmount < 0.01f || strand.positions.size() < 2)
        return strand;  // No twist needed

    Strand twistedStrand = strand;

    // For each position, rotate around the strand's centerline
    for (size_t i = 1; i < twistedStrand.positions.size() - 1; ++i)
    {
        float t = static_cast<float>(i) / (twistedStrand.positions.size() - 1);

        // Rotation amount increases along strand
        float angle = twistAmount * t * twistFrequency * 3.14159f / 180.0f;

        // Get strand direction at this point
        UT_Vector3 direction = twistedStrand.positions[i + 1] - twistedStrand.positions[i - 1];
        direction.normalize();

        // Rotate around strand axis
        twistedStrand.positions[i] = rotateAroundAxis(
            twistedStrand.positions[i],
            direction,
            twistedStrand.positions[i],
            angle);
    }

    return twistedStrand;
}

Strand ClumpOperator::applyNoise(
    const Strand& strand,
    float noiseFrequency,
    float noiseAmplitude,
    float noiseScale)
{
    if (noiseAmplitude < 0.01f || strand.positions.size() < 2)
        return strand;  // No noise needed

    Strand noisyStrand = strand;

    for (size_t i = 0; i < noisyStrand.positions.size(); ++i)
    {
        // Compute noise value at this position
        float x = noisyStrand.positions[i].x() * noiseFrequency;
        float y = noisyStrand.positions[i].y() * noiseFrequency;
        float z = static_cast<float>(i) * noiseFrequency;

        // Get 3D Perlin noise
        float noise = perlinNoise3D(x, y, z);

        // Map to [-1, 1] range
        noise = (noise * 2.0f) - 1.0f;

        // Apply noise as displacement
        UT_Vector3 displacement(
            noise * noiseAmplitude * noiseScale,
            perlinNoise3D(x + 17.5f, y + 31.2f, z + 43.7f) * 2.0f - 1.0f,
            perlinNoise3D(x + 61.3f, y + 89.4f, z + 23.6f) * 2.0f - 1.0f);

        displacement.x() *= (noiseAmplitude * noiseScale);
        displacement.y() *= (noiseAmplitude * noiseScale);
        displacement.z() *= (noiseAmplitude * noiseScale);

        noisyStrand.positions[i] += displacement;
    }

    return noisyStrand;
}

Strand ClumpOperator::applyBend(
    const Strand& strand,
    const UT_Vector3& bendDirection,
    float bendMagnitude)
{
    if (bendMagnitude < 0.01f || bendDirection.length() < 0.01f || strand.positions.size() < 2)
        return strand;  // No bend needed

    Strand bentStrand = strand;

    // Normalize bend direction
    UT_Vector3 bendDir = bendDirection;
    bendDir.normalize();

    for (size_t i = 0; i < bentStrand.positions.size(); ++i)
    {
        // Amount of bending increases along strand
        float t = static_cast<float>(i) / std::max(1, (int)bentStrand.positions.size() - 1);

        // Apply bend as progressive displacement
        UT_Vector3 bendDisplacement;
        bendDisplacement.x() = bendDir.x() * (t * bendMagnitude * 0.5f);
        bendDisplacement.y() = bendDir.y() * (t * bendMagnitude * 0.5f);
        bendDisplacement.z() = bendDir.z() * (t * bendMagnitude * 0.5f);

        bentStrand.positions[i] += bendDisplacement;
    }

    return bentStrand;
}

Strand ClumpOperator::applyAllTransformations(
    const Strand& strand,
    const ClumpOperatorParams& params)
{
    Strand result = strand;

    // Apply transformations in sequence:
    // 1. Twist (rotate around centerline)
    if (params.twistAmount > 0.01f)
    {
        result = applyTwist(result, params.twistAmount, params.twistFrequency);
    }

    // 2. Noise (add variation)
    if (params.noiseAmplitude > 0.01f)
    {
        result = applyNoise(result, params.noiseFrequency, params.noiseAmplitude, params.noiseScale);
    }

    // 3. Bend (apply directional deformation)
    if (params.bendMagnitude > 0.01f)
    {
        result = applyBend(result, params.bendDirection, params.bendMagnitude);
    }

    // Recompute arc length after transformations
    result.arcLength = 0.0f;
    for (size_t i = 1; i < result.positions.size(); ++i)
    {
        float segLen = (result.positions[i] - result.positions[i - 1]).length();
        result.arcLength += segLen;
    }

    return result;
}
