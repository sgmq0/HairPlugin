#include "LossComputation.h"
#include <cmath>
#include <algorithm>

LossComponents LossComputation::computeLoss(
    const StrandSet& inputStrands,
    const GuideSet& guides,
    const StrandSet& synthesizedStrands)
{
    LossComponents loss;

    loss.reconstructionError = computeReconstructionError(inputStrands, synthesizedStrands);
    loss.guideSmoothness = computeGuideSmoothness(guides);
    
    // Weighted combination
    loss.totalLoss = 0.7f * loss.reconstructionError + 0.3f * loss.guideSmoothness;

    return loss;
}

float LossComputation::computeReconstructionError(
    const StrandSet& inputStrands,
    const StrandSet& synthesizedStrands)
{
    if (inputStrands.getStrandCount() == 0 || synthesizedStrands.getStrandCount() == 0) {
        return 1000.0f;  // High error if nothing to compare
    }

    float totalError = 0.0f;
    int count = 0;

    // For each input strand, find closest synthesized strand
    for (int i = 0; i < inputStrands.getStrandCount(); ++i) {
        const Strand& input = inputStrands.getStrand(i);
        
        if (input.positions.empty()) continue;

        float minDist = 1e10f;

        // Find closest synthesized strand
        for (int j = 0; j < synthesizedStrands.getStrandCount(); ++j) {
            const Strand& synth = synthesizedStrands.getStrand(j);
            
            if (synth.positions.empty()) continue;

            // Distance between root positions
            UT_Vector3 diff = input.positions[0] - synth.positions[0];
            float dist = diff.length();
            minDist = std::min(minDist, dist);
        }

        totalError += minDist;
        count++;
    }

    return (count > 0) ? totalError / count : 1000.0f;
}

float LossComputation::computeGuideSmoothness(
    const GuideSet& guides)
{
    if (guides.getGuideCount() == 0) {
        return 0.0f;
    }

    float totalCurvature = 0.0f;
    int count = 0;

    // Penalize high curvature in guides
    for (int i = 0; i < guides.getGuideCount(); ++i) {
        const Strand& guide = guides.getGuide(i);
        
        if (guide.positions.size() < 3) continue;

        for (size_t j = 1; j < guide.positions.size() - 1; ++j) {
            UT_Vector3 v1 = guide.positions[j] - guide.positions[j - 1];
            UT_Vector3 v2 = guide.positions[j + 1] - guide.positions[j];

            float len1 = v1.length();
            float len2 = v2.length();

            if (len1 > 1e-6f && len2 > 1e-6f) {
                v1 /= len1;
                v2 /= len2;
                float dot = v1.dot(v2);
                float angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));
                totalCurvature += angle;
                count++;
            }
        }
    }

    return (count > 0) ? totalCurvature / count : 0.0f;
}
