#ifndef SIMPLE_OPTIMIZER_H
#define SIMPLE_OPTIMIZER_H

#include "StrandSet.h"
#include "GuideSet.h"
#include "ClumpOperator.h"
#include "LossComputation.h"

class SimpleOptimizer
{
public:
    SimpleOptimizer();

    // Perform one optimization step
    void step(
        GuideSet& guides,
        float& clumpRadius,
        float& clumpTightness,
        int& clumpCount,
        const StrandSet& inputStrands,
        StrandSet& synthesizedStrands);

    // Get current loss
    LossComponents getCurrentLoss() const { return currentLoss; }

    // Check if loss improved
    bool isImproving() const;

    // Get optimized synthesized strands
    StrandSet getSynthesizedStrands() const { return synthesizedStrands; }

private:
    LossComponents currentLoss;
    LossComponents previousLoss;
    StrandSet synthesizedStrands;
    int iterationsWithoutImprovement = 0;
    static constexpr int PATIENCE = 5;
};

#endif