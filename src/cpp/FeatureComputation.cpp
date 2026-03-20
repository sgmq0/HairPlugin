#include "FeatureComputation.h"
#include <cmath>
#include <algorithm>

std::vector<Eigen::VectorXf> FeatureComputation::computeAllFeatures(
    const StrandSet& strands)
{
    std::vector<Eigen::VectorXf> features;
    features.reserve(strands.getStrandCount());

    for (int i = 0; i < strands.getStrandCount(); ++i)
    {
        const Strand& strand = strands.getStrand(i);
        features.push_back(computeStrandFeature(strand));
    }

    normalizeFeatures(features);

    return features;
}

Eigen::VectorXf FeatureComputation::computeStrandFeature(const Strand& strand)
{
    Eigen::VectorXf feat(FEATURE_DIM);

    UT_Vector3 rootPos = strand.positions.empty() ? UT_Vector3(0, 0, 0) :
        strand.positions.front();
    feat(0) = rootPos.x();
    feat(1) = rootPos.y();
    feat(2) = rootPos.z();

    UT_Vector3 tipDir = strand.getTipDirection();
    feat(3) = tipDir.x();
    feat(4) = tipDir.y();
    feat(5) = tipDir.z();

    feat(6) = strand.arcLength;

    return feat;
}

void FeatureComputation::normalizeFeatures(std::vector<Eigen::VectorXf>& features)
{
    if (features.empty())
        return;

    Eigen::VectorXf mean = Eigen::VectorXf::Zero(FEATURE_DIM);
    for (const auto& feat : features)
        mean += feat;
    mean /= static_cast<float>(features.size());

    Eigen::VectorXf variance = Eigen::VectorXf::Zero(FEATURE_DIM);
    for (const auto& feat : features)
        variance += (feat - mean).array().square().matrix();
    variance /= static_cast<float>(features.size());

    Eigen::VectorXf stddev = variance.array().sqrt();

    const float EPSILON = 1e-6f;
    for (int i = 0; i < FEATURE_DIM; ++i)
        if (stddev(i) < EPSILON)
            stddev(i) = 1.0f;

    for (auto& feat : features)
    {
        for (int i = 0; i < FEATURE_DIM; ++i)
        {
            feat(i) = (feat(i) - mean(i)) / stddev(i);
        }
    }
}