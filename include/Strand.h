#ifndef STRAND_H
#define STRAND_H

#include <UT/UT_Vector3.h>
#include <vector>

struct Strand
{
    std::vector<UT_Vector3> positions;
    std::vector<float> radius;
    float arcLength;
    int clusterID;
    float reconstructionError;

    Strand();
    ~Strand();

    void normalize();
    void resample(int numPoints);

    UT_Vector3 getTipDirection() const;
    float computeAverageRadius() const;
    float computeAverageCurvature() const;
    UT_Vector3 getRoot() const;
};

#endif