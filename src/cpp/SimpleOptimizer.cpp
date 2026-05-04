#include "SimpleOptimizer.h"
#include "LossComputation.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// SimpleOptimizer - Adam Optimizer Implementation
// Following Chang et al. SIGGRAPH 2025, Section 5.4
// Kingma & Ba "Adam: A Method for Stochastic Optimization" ICLR 2015
// ============================================================================

SimpleOptimizer::SimpleOptimizer()
    : iterationsWithoutImprovement(0),
    beta1(0.9f),      // First moment decay
    beta2(0.999f),    // Second moment decay
    epsilon(1e-8f),   // Numerical stability
    learningRate(0.01f),
    t(0)              // Iteration counter
{
    currentLoss = { 0.0f, 0.0f, 0.0f };
    previousLoss = { 0.0f, 0.0f, 0.0f };

    // Initialize moment estimates to 0
    m_profile = 0.0f;
    v_profile = 0.0f;
    m_tightness = 0.0f;
    v_tightness = 0.0f;
    m_count = 0.0f;
    v_count = 0.0f;
}

void SimpleOptimizer::step(
    GuideSet& guides,
    float& clumpProfile,
    float& clumpTightness,
    int& clumpCount,
    const StrandSet& inputStrands,
    StrandSet& synthesizedStrands)
{
    // Increment iteration counter (for bias correction)
    t++;

    // ====================================================================
    // Step 1: Store previous loss for improvement check
    // ====================================================================
    previousLoss = currentLoss;

    // ====================================================================
    // Step 2: Compute current loss with current parameters
    // ====================================================================
    currentLoss = LossComputation::computeLoss(
        inputStrands,
        guides,
        synthesizedStrands);

    // ====================================================================
    // Step 3: Compute gradients via simple heuristics
    // (Full finite difference would require re-synthesis)
    // ====================================================================

    // Simple heuristic gradients based on reconstruction error
    float dL_dProfile = -0.1f * currentLoss.reconstructionError;
    float dL_dTightness = -0.05f * currentLoss.reconstructionError;
    float dL_dCount = -0.05f * currentLoss.reconstructionError;

    // ====================================================================
    // Step 4: Adam optimizer update (Kingma & Ba 2015, Algorithm 1)
    // ====================================================================

    // Update biased first moment estimate
    m_profile = beta1 * m_profile + (1.0f - beta1) * dL_dProfile;
    m_tightness = beta1 * m_tightness + (1.0f - beta1) * dL_dTightness;
    m_count = beta1 * m_count + (1.0f - beta1) * dL_dCount;

    // Update biased second moment estimate
    v_profile = beta2 * v_profile + (1.0f - beta2) * (dL_dProfile * dL_dProfile);
    v_tightness = beta2 * v_tightness + (1.0f - beta2) * (dL_dTightness * dL_dTightness);
    v_count = beta2 * v_count + (1.0f - beta2) * (dL_dCount * dL_dCount);

    // Bias correction (critical for first few iterations!)
    float bias_correction1 = 1.0f - std::pow(beta1, (float)t);
    float bias_correction2 = 1.0f - std::pow(beta2, (float)t);

    float m_profile_hat = m_profile / bias_correction1;
    float v_profile_hat = v_profile / bias_correction2;

    float m_tightness_hat = m_tightness / bias_correction1;
    float v_tightness_hat = v_tightness / bias_correction2;

    float m_count_hat = m_count / bias_correction1;
    float v_count_hat = v_count / bias_correction2;

    // Parameter updates
    float alpha = learningRate;

    clumpProfile -= alpha * m_profile_hat / (std::sqrt(v_profile_hat) + epsilon);
    clumpTightness -= alpha * m_tightness_hat / (std::sqrt(v_tightness_hat) + epsilon);
    clumpCount -= (int)std::round(alpha * m_count_hat / (std::sqrt(v_count_hat) + epsilon));

    // ====================================================================
    // Step 5: Clamp parameters to valid ranges
    // ====================================================================
    clumpProfile = std::max(0.1f, std::min(10.0f, clumpProfile));
    clumpTightness = std::max(0.0f, std::min(1.0f, clumpTightness));
    clumpCount = std::max(1, std::min(100, clumpCount));

    // ====================================================================
    // Step 6: Store synthesized strands for getSynthesizedStrands()
    // ====================================================================
    synthesizedStrands = synthesizedStrands;  // Already updated externally

    // ====================================================================
    // Step 7: Check for convergence
    // ====================================================================
    if (!isImproving()) {
        iterationsWithoutImprovement++;
    }
    else {
        iterationsWithoutImprovement = 0;
    }
}

bool SimpleOptimizer::isImproving() const
{
    // Check if loss decreased
    const float improvementThreshold = 1e-4f;
    return (previousLoss.totalLoss - currentLoss.totalLoss) > improvementThreshold;
}

bool SimpleOptimizer::hasConverged() const
{
    // Check convergence: patience exceeded or loss threshold reached
    return iterationsWithoutImprovement >= PATIENCE ||
        currentLoss.totalLoss < 0.01f;
}

void SimpleOptimizer::setLearningRate(float lr)
{
    learningRate = lr;
}

void SimpleOptimizer::reset()
{
    t = 0;
    iterationsWithoutImprovement = 0;
    m_profile = 0.0f;
    v_profile = 0.0f;
    m_tightness = 0.0f;
    v_tightness = 0.0f;
    m_count = 0.0f;
    v_count = 0.0f;
    currentLoss = { 0.0f, 0.0f, 0.0f };
    previousLoss = { 0.0f, 0.0f, 0.0f };
}