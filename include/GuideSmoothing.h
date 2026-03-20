#ifndef GUIDE_SMOOTHING_H
#define GUIDE_SMOOTHING_H

#include "Strand.h"
#include <vector>

class GuideSmoothing
{
public:
    static Strand smoothGuideCurve(const Strand& rawGuide, int numControlPoints = 10);

private:
    static float basisFunction(int i, int p, float t, const std::vector<float>& knots);
};

#endif