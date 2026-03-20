#include "GuideSmoothing.h"
#include <algorithm>
#include <cmath>

Strand GuideSmoothing::smoothGuideCurve(
    const Strand& rawGuide,
    int numControlPoints)
{
    Strand smoothedGuide;

    if (rawGuide.positions.size() < 2)
        return rawGuide;

    // Use input points as control points
    if (rawGuide.positions.size() <= (size_t)numControlPoints)
    {
        // If we have fewer points than desired controls, just return as-is
        smoothedGuide = rawGuide;
    }
    else
    {
        // Downsample to get control points evenly distributed
        std::vector<UT_Vector3> controlPoints;
        controlPoints.reserve(numControlPoints);

        float step = static_cast<float>(rawGuide.positions.size() - 1) /
            (numControlPoints - 1);

        for (int i = 0; i < numControlPoints; ++i)
        {
            float idx = i * step;
            int prevIdx = static_cast<int>(idx);
            int nextIdx = std::min(prevIdx + 1,
                static_cast<int>(rawGuide.positions.size()) - 1);

            float t = idx - prevIdx;
            UT_Vector3 controlPt =
                rawGuide.positions[prevIdx] * (1.0f - t) +
                rawGuide.positions[nextIdx] * t;

            controlPoints.push_back(controlPt);
        }

        // Interpolate between control points to create smooth curve
        smoothedGuide.positions.clear();
        smoothedGuide.positions.reserve(rawGuide.positions.size());

        for (size_t i = 0; i < controlPoints.size() - 1; ++i)
        {
            int pointsInSegment = static_cast<int>(
                rawGuide.positions.size() / (numControlPoints - 1));

            for (int j = 0; j < pointsInSegment; ++j)
            {
                float t = static_cast<float>(j) / pointsInSegment;
                UT_Vector3 interpolated =
                    controlPoints[i] * (1.0f - t) +
                    controlPoints[i + 1] * t;

                smoothedGuide.positions.push_back(interpolated);
            }
        }

        // Add last control point
        smoothedGuide.positions.push_back(controlPoints.back());
    }

    // Copy other properties
    smoothedGuide.radius.resize(smoothedGuide.positions.size(), 1.0f);
    smoothedGuide.clusterID = rawGuide.clusterID;

    // Recompute arc length
    smoothedGuide.arcLength = 0.0f;
    for (size_t i = 1; i < smoothedGuide.positions.size(); ++i)
    {
        float segLen = (smoothedGuide.positions[i] -
            smoothedGuide.positions[i - 1]).length();
        smoothedGuide.arcLength += segLen;
    }

    return smoothedGuide;
}

float GuideSmoothing::basisFunction(int i, int p, float t,
    const std::vector<float>& knots)
{
    // Cox-de Boor recursion formula for B-spline basis
    if (p == 0)
    {
        return (knots[i] <= t && t < knots[i + 1]) ? 1.0f : 0.0f;
    }

    float left = 0.0f, right = 0.0f;

    float denom_left = knots[i + p] - knots[i];
    if (denom_left > 1e-6f)
        left = (t - knots[i]) / denom_left *
        basisFunction(i, p - 1, t, knots);

    float denom_right = knots[i + p + 1] - knots[i + 1];
    if (denom_right > 1e-6f)
        right = (knots[i + p + 1] - t) / denom_right *
        basisFunction(i + 1, p - 1, t, knots);

    return left + right;
}