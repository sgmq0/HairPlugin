#ifndef SIMPLE_OPTIMIZER_H
#define SIMPLE_OPTIMIZER_H

#include "StrandSet.h"
#include "GuideSet.h"
#include "LossComputation.h"

// ============================================================================
// SimpleOptimizer - Adam Optimizer for Clump Parameter Optimization
// Following: Chang et al. SIGGRAPH 2025, Section 5.4
// Implements: Kingma & Ba, "Adam: A Method for Stochastic Optimization" 
//             ICLR 2015
// ============================================================================

class SimpleOptimizer
{
public:
    SimpleOptimizer();

    // Perform one optimization step
    // Updates clumpProfile, clumpTightness, and clumpCount to reduce loss
    void step(
        GuideSet& guides,
        float& clumpProfile,
        float& clumpTightness,
        int& clumpCount,
        const StrandSet& inputStrands,
        StrandSet& synthesizedStrands);

    // Get current loss value
    LossComponents getCurrentLoss() const { return currentLoss; }

    // Check if loss improved this iteration
    bool isImproving() const;

    // Check convergence (loss threshold or patience exceeded)
    bool hasConverged() const;

    // Get the synthesized strands (copy of current synthesized hair)
    StrandSet getSynthesizedStrands() const { return synthesizedStrands; }

    // Configuration
    void setLearningRate(float lr);
    void reset();

private:
    // Loss tracking
    LossComponents currentLoss;
    LossComponents previousLoss;
    StrandSet synthesizedStrands;
    int iterationsWithoutImprovement = 0;
    static constexpr int PATIENCE = 5;

    // ====================================================================
    // Adam Optimizer State (Kingma & Ba 2015)
    // ====================================================================

    float beta1;          // First moment decay (0.9)
    float beta2;          // Second moment decay (0.999)
    float epsilon;        // Numerical stability (1e-8)
    float learningRate;   // Step size (0.01)
    int t;                // Iteration counter for bias correction

    // First moment estimates (exponential moving average of gradients)
    float m_profile;
    float m_tightness;
    float m_count;

    // Second moment estimates (exponential moving average of squared gradients)
    float v_profile;
    float v_tightness;
    float v_count;
};

#endif