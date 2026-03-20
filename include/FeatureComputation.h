#ifndef FEATURE_COMPUTATION_H
#define FEATURE_COMPUTATION_H

#include "Strand.h"
#include "StrandSet.h"
#include <eigen-5.0.0/Eigen/Dense>
#include <vector>

class FeatureComputation
{
public:
    static std::vector<Eigen::VectorXf> computeAllFeatures(const StrandSet& strands);
    static void normalizeFeatures(std::vector<Eigen::VectorXf>& features);
    static Eigen::VectorXf computeStrandFeature(const Strand& strand);

private:
    static const int FEATURE_DIM = 7;
};

#endif