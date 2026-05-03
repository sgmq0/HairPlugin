#ifndef STRAND_H
#define STRAND_H

#include <UT/UT_Vector3.h>
#include <SOP/SOP_Node.h>
#include <GU/GU_RayIntersect.h>

#include <vector>

class GuideSet;

struct Strand
{
    std::vector<UT_Vector3> positions;
    std::vector<UT_Vector3> deformedPositions;

    std::vector<float> radius;
    float arcLength;
    int clusterID;
    float reconstructionError;
    UT_Vector2 root_UV;
    std::vector<std::pair<float, int>> guides;  // (distance, index) n guide strands that influence it
    std::vector<std::pair<float, int>> guideWeights;    // (weight, index) vector of weights for the guides

    // clump operatorrr
    float bendRand;

    Strand();
    ~Strand();

    void normalize();
    void resample(int numPoints);

    UT_Vector3 getTipDirection() const;
    float computeAverageRadius() const;
    float computeAverageCurvature() const;
    void computeUVLocation(GU_Detail* gdp, GU_MinInfo* minInfo);
    void computeGuideWeights(GuideSet* guideSet);
    UT_Vector3 getRoot() const;

};

#endif