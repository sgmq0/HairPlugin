#ifndef LOSS_COMPUTATION_H
#define LOSS_COMPUTATION_H

#include "StrandSet.h"
#include "GuideSet.h"
#include <string>
#include <vector>

// ============================================================================
// Loss Components Structure
// ============================================================================

struct LossComponents
{
    float reconstructionError = 0.0f;  // L_x: matches target distribution
    float guideSmoothness = 0.0f;      // L_smoothness: penalizes curvature
    float totalLoss = 0.0f;            // Weighted combination
};

// ============================================================================
// LossComputation - Multi-Objective Loss Functions
// Following: Chang et al. "Transforming Unstructured Hair Strands into 
//            Procedural Hair Grooms" SIGGRAPH 2025, Section 5
// ============================================================================

class LossComputation
{
public:
    // Main loss computation (Section 5.4)
    // L_total = 0.7 * L_reconstruction + 0.3 * L_smoothness
    // Uses probabilistic view: all strands are samples from P_θ(g, r_i)
    static LossComponents computeLoss(
        const StrandSet& inputStrands,
        const GuideSet& guides,
        const StrandSet& synthesizedStrands);

    // ====================================================================
    // Guide Optimization Loss (Section 5.4)
    // ====================================================================

    // Reconstruction error: how well synthesized strands match input
    // L_x = (1/n) Σ_i ||x̄_i(g, θ) - x̂_i||²
    // where x̄_i = E_u[P_θ(g, r_i)] is expected strand from procedural model
    static float computeReconstructionError(
        const StrandSet& inputStrands,
        const StrandSet& synthesizedStrands);

    // Guide smoothness: penalize high curvature (variation baking)
    // Discrete Laplacian smoothness (Eq. 6)
    // Encourages guides to be smooth with variation added by operators
    static float computeGuideSmoothness(
        const GuideSet& guides);

    // ====================================================================
    // Operator Parameter Optimization (Section 5.5)
    // ====================================================================

    // Sliced Wasserstein Distance for strand shape operators
    // (Section 5.5, Eq. 11-12)
    // Used for: curl, bend, frizz operators
    // Compares distributions via random projections and sorting
    static float computeSlicedWassersteinLoss(
        const StrandSet& estimatedStrands,
        const StrandSet& targetStrands,
        int numSlices = 50);

    // Determinantal Point Process Loss for strand correlation operators
    // (Section 5.5, Eq. 13)
    // Used for: clump, scale operators
    // Measures clustering (DPP determinant encodes point repulsion)
    static float computeDPPLoss(
        const StrandSet& estimatedStrands,
        const StrandSet& targetStrands,
        const std::string& operatorType);

private:
    // Helper: extract strand segment features for Wasserstein distance
    static std::vector<UT_Vector3> extractStrandFeatures(
        const StrandSet& strands);

    // Helper: compute strand correlation for DPP loss
    static float computeStrandCorrelation(
        const StrandSet& strands,
        const std::string& operatorType);
};

#endif