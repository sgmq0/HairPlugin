#include "FeatureComputation.h"
#include <cmath>
#include <algorithm>

Feature FeatureComputation::computeStrandFeature(const Strand& strand)
{
    Feature feature;

    // Root position (x, y, z)
    const UT_Vector3& root = strand.positions[0];
    feature.values[0] = root.x();
    feature.values[1] = root.y();
    feature.values[2] = root.z();

    // Tip direction (x, y, z)
    UT_Vector3 tipDir = strand.getTipDirection();
    feature.values[3] = tipDir.x();
    feature.values[4] = tipDir.y();
    feature.values[5] = tipDir.z();

    // Arc length
    feature.values[6] = strand.arcLength;

    return feature;
}

std::vector<Feature> FeatureComputation::computeAllFeatures(const StrandSet& strands)
{
    std::vector<Feature> features;

    for (int i = 0; i < strands.getStrandCount(); ++i)
    {
        const Strand& strand = strands.getStrand(i);
        Feature feature = computeStrandFeature(strand);
        features.push_back(feature);
    }

    return features;
}

void FeatureComputation::normalizeFeatures(std::vector<Feature>& features)
{
    if (features.empty())
        return;

    const int featureDim = 7;
    const int numFeatures = features.size();

    // Compute mean for each dimension
    float mean[featureDim];
    for (int d = 0; d < featureDim; ++d)
    {
        mean[d] = 0.0f;
    }

    for (const auto& feature : features)
    {
        for (int d = 0; d < featureDim; ++d)
        {
            mean[d] += feature.values[d];
        }
    }

    for (int d = 0; d < featureDim; ++d)
    {
        mean[d] /= numFeatures;
    }

    // Compute standard deviation for each dimension
    float stdDev[featureDim];
    for (int d = 0; d < featureDim; ++d)
    {
        stdDev[d] = 0.0f;
    }

    for (const auto& feature : features)
    {
        for (int d = 0; d < featureDim; ++d)
        {
            float diff = feature.values[d] - mean[d];
            stdDev[d] += diff * diff;
        }
    }

    for (int d = 0; d < featureDim; ++d)
    {
        stdDev[d] = std::sqrt(stdDev[d] / numFeatures);
    }

    // Normalize: (x - mean) / stdDev
    for (auto& feature : features)
    {
        for (int d = 0; d < featureDim; ++d)
        {
            float divisor = (stdDev[d] > 1e-6f) ? stdDev[d] : 1e-6f;
            feature.values[d] = (feature.values[d] - mean[d]) / divisor;
        }
    }
}