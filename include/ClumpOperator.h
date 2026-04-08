#ifndef CLUMP_OPERATOR_H
#define CLUMP_OPERATOR_H

#include "StrandSet.h"
#include "GuideSet.h"
#include <vector>

class ClumpOperator
{
public:
    // Main function: Generate clumped strands from guides
    static StrandSet clumpFromGuides(
        const GuideSet& guides,
        float clumpRadius,
        float clumpTightness,
        int clumpCount);

private:
    // Helper: Generate strands around a single guide
    static std::vector<Strand> clumpAroundGuide(
        const Strand& guide,
        float clumpRadius,
        float clumpTightness,
        int clumpCount);

    // Helper: Create a single clumped strand offset from guide
    static Strand createClumpedStrand(
        const Strand& guide,
        float offsetRadius,
        float tightness);

    // Helper: Generate random offset vector with controlled spread
    static UT_Vector3 generateRandomOffset(
        float maxRadius,
        float tightness);
};

#endif
