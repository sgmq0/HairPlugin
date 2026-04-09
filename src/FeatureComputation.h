#ifndef FEATURE_COMPUTATION_H
#define FEATURE_COMPUTATION_H

#include "StrandSet.h"
#include <vector>

// Simple feature vector - just use float arrays instead of Eigen
struct Feature
{
    std::array<float,7> values;  // 7D feature vector

    Feature() { for (int i = 0; i < 7; ++i) values[i] = 0.0f; }

    float featureDistance(const Feature& b); // helper function to calc the distance between two feature vectors
};

class FeatureComputation
{
public:
    // Compute feature vector for a single strand
    static Feature computeStrandFeature(const Strand& strand);

    // Compute feature vectors for all strands in a StrandSet
    static std::vector<Feature> computeAllFeatures(const StrandSet& strands);

    // Normalize features to zero mean and unit variance
    static void normalizeFeatures(std::vector<Feature>& features);

private:
    FeatureComputation();
};

#endif