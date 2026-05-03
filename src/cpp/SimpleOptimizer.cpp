#include "SimpleOptimizer.h"
#include <algorithm>

SimpleOptimizer::SimpleOptimizer()
{
    currentLoss = { 0.0f, 0.0f, 0.0f };
    previousLoss = { 0.0f, 0.0f, 0.0f };
}

void SimpleOptimizer::step(
    GuideSet& guides,
    float& clumpProfile,
    float& clumpTightness,
    int& clumpCount,
    const StrandSet& inputStrands,
    StrandSet& synthesizedStrands)
{
    // Store previous loss
    previousLoss = currentLoss;

    // Re-synthesize with current parameters
    ClumpParams params;
    params.clumpProfile = clumpProfile;

    // Store in both member and parameter reference
    /*this->synthesizedStrands = ClumpOperator::clumpFromGuides(guides, params);
    synthesizedStrands = this->synthesizedStrands;*/

    // Compute new loss
    currentLoss = LossComputation::computeLoss(
        inputStrands,
        guides,
        synthesizedStrands);

    // Simple parameter adjustment based on loss
    if (!isImproving()) {
        iterationsWithoutImprovement++;

        if (iterationsWithoutImprovement < PATIENCE) {
            // Try tweaking parameters
            clumpProfile *= 0.95f;
            clumpTightness *= 1.05f;
        }
    }
    else {
        iterationsWithoutImprovement = 0;
    }

    // Clamp parameters to valid ranges
    clumpProfile = std::max(0.1f, std::min(10.0f, clumpProfile));
    clumpTightness = std::max(0.0f, std::min(1.0f, clumpTightness));
    clumpCount = std::max(1, std::min(100, clumpCount));
}

bool SimpleOptimizer::isImproving() const
{
    // Check if loss decreased
    return currentLoss.totalLoss < previousLoss.totalLoss;
}