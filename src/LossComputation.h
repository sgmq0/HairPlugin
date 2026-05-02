#ifndef LOSS_COMPUTATION_H
#define LOSS_COMPUTATION_H

#include "StrandSet.h"
#include "GuideSet.h"

struct LossComponents
{
    float reconstructionError = 0.0f;
    float guideSmoothness = 0.0f;
    float totalLoss = 0.0f;
};

class LossComputation
{
public:
    // Compute loss between input and synthesized strands
    static LossComponents computeLoss(
        const StrandSet& inputStrands,
        const GuideSet& guides,
        const StrandSet& synthesizedStrands);

private:
    // Compute how well synthesized strands cover input
    static float computeReconstructionError(
        const StrandSet& inputStrands,
        const StrandSet& synthesizedStrands);

    // Compute smoothness penalty on guide curves
    static float computeGuideSmoothness(
        const GuideSet& guides);
};

#endif
