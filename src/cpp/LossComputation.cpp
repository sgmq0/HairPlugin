#include "LossComputation.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================================================================
// LossComputation - Multi-Objective Loss Functions
// Following Chang et al. SIGGRAPH 2025, Section 5
// ============================================================================

LossComponents LossComputation::computeLoss(
    const StrandSet& inputStrands,
    const GuideSet& guides,
    const StrandSet& synthesizedStrands)
{
    LossComponents loss;

    // Probabilistic view (Section 5.1): 
    // Procedural groom generates distribution P_θ of strands
    // All strands (input + synthetic) are samples from distributions
    // Goal: match E[P_θ] ≈ E[P_θ̂] (expected strand distributions)

    loss.reconstructionError = computeReconstructionError(inputStrands, synthesizedStrands);
    loss.guideSmoothness = computeGuideSmoothness(guides);

    // Weighted loss combination (typical weights from practice)
    // Focus: reconstruction with smoothness penalty
    loss.totalLoss = 0.7f * loss.reconstructionError + 0.3f * loss.guideSmoothness;

    return loss;
}

float LossComputation::computeReconstructionError(
    const StrandSet& inputStrands,
    const StrandSet& synthesizedStrands)
{
    // Section 5.4: Guide Optimization Loss
    // L_g = (1/n) Σ_i || E_u[P_θ(g, r_i)] - E_û[P_θ̂(ĝ, r̂_i)] ||²
    // 
    // Probabilistic matching: guides represent distribution means
    // Not vertex-wise matching (which bakes variation into guides)
    // Instead, match overall structure via root+direction matching

    if (inputStrands.getStrandCount() == 0 || synthesizedStrands.getStrandCount() == 0) {
        return 1000.0f;  // High error if nothing to compare
    }

    float totalError = 0.0f;
    int count = 0;

    // For each input strand, find closest synthesized strand
    // This approximates optimal transport matching
    for (int i = 0; i < inputStrands.getStrandCount(); ++i) {
        const Strand& input = inputStrands.getStrand(i);

        if (input.positions.empty()) continue;

        float minDist = 1e10f;
        float bestStrandError = 0.0f;

        // Find closest synthesized strand
        for (int j = 0; j < synthesizedStrands.getStrandCount(); ++j) {
            const Strand& synth = synthesizedStrands.getStrand(j);

            if (synth.positions.empty()) continue;

            // Root distance: primary matching criterion
            UT_Vector3 rootDiff = input.positions[0] - synth.positions[0];
            float rootDist = rootDiff.length();

            if (rootDist < minDist) {
                minDist = rootDist;

                // Compute vertex-wise error for best matching pair
                float strandError = 0.0f;
                int minSize = std::min(input.positions.size(), synth.positions.size());

                for (int k = 0; k < minSize; ++k) {
                    UT_Vector3 diff = input.positions[k] - synth.positions[k];
                    strandError += diff.length();
                }
                strandError /= minSize;
                bestStrandError = strandError;
            }
        }

        // Use best match error
        totalError += bestStrandError;
        count++;
    }

    return (count > 0) ? totalError / count : 1000.0f;
}

float LossComputation::computeGuideSmoothness(
    const GuideSet& guides)
{
    // Section 5.4: Guide Smoothness Penalty
    // Prevents variation from being baked into guide curves
    // Uses discrete Laplacian (Eq. 6 in paper)
    //
    // The heat equation iterative smoothing:
    // (I + λ_smooth * L) * x^{t+1} = x^t
    // Where L is discrete Laplacian
    // 
    // We penalize curvature: high curvature = high angle changes

    if (guides.getGuideCount() == 0) {
        return 0.0f;
    }

    float totalCurvature = 0.0f;
    int count = 0;

    // Penalize high curvature in guides
    for (int i = 0; i < guides.getGuideCount(); ++i) {
        const Strand& guide = guides.getGuide(i);

        if (guide.positions.size() < 3) continue;

        // Curvature: angle between consecutive segments
        for (size_t j = 1; j < guide.positions.size() - 1; ++j) {
            UT_Vector3 v1 = guide.positions[j] - guide.positions[j - 1];
            UT_Vector3 v2 = guide.positions[j + 1] - guide.positions[j];

            float len1 = v1.length();
            float len2 = v2.length();

            if (len1 > 1e-6f && len2 > 1e-6f) {
                v1 /= len1;
                v2 /= len2;

                // Dot product of normalized vectors
                float dot = v1.dot(v2);
                dot = std::max(-1.0f, std::min(1.0f, dot));

                // Angle from dot product (curvature angle)
                float angle = std::acos(dot);

                // Square penalty (encourage small angles)
                totalCurvature += angle * angle;
                count++;
            }
        }
    }

    return (count > 0) ? totalCurvature / count : 0.0f;
}

// ============================================================================
// Sliced Wasserstein Distance (Section 5.5)
// For comparing strand distributions without known correspondences
// ============================================================================

float LossComputation::computeSlicedWassersteinLoss(
    const StrandSet& estimatedStrands,
    const StrandSet& targetStrands,
    int numSlices)
{
    // Section 5.5: Strand Shape Operator Optimization
    // SW(f, f̂) = (1/s) Σ_{k=1}^s ||sort({<v_k, f_i>}) - sort({<v_k, f̂_i>})||²
    //
    // Where:
    // - f_i = h(x_i) are features extracted from strands
    // - v_k are random projection vectors
    // - Sorting implicitly computes optimal transport matching
    //
    // Used for: curl, bend, frizz operators (modify strand shape)
    // Advantage: handles unknown correspondences, invariant to permutations

    if (estimatedStrands.getStrandCount() == 0 || targetStrands.getStrandCount() == 0) {
        return 1000.0f;
    }

    float totalLoss = 0.0f;

    // Extract features (strand segments)
    std::vector<UT_Vector3> estFeatures = extractStrandFeatures(estimatedStrands);
    std::vector<UT_Vector3> targetFeatures = extractStrandFeatures(targetStrands);

    if (estFeatures.empty() || targetFeatures.empty()) {
        return 1000.0f;
    }

    // Random projections
    for (int k = 0; k < numSlices; ++k) {
        // Random direction v_k on unit sphere
        UT_Vector3 vk(
            (float)rand() / RAND_MAX - 0.5f,
            (float)rand() / RAND_MAX - 0.5f,
            (float)rand() / RAND_MAX - 0.5f
        );
        float vkLen = vk.length();
        if (vkLen > 1e-6f) vk /= vkLen;

        // Project features onto v_k: <v_k, f_i>
        std::vector<float> estProj, targetProj;

        for (const auto& f : estFeatures) {
            estProj.push_back(f.dot(vk));
        }

        for (const auto& f : targetFeatures) {
            targetProj.push_back(f.dot(vk));
        }

        // Sort projections
        std::sort(estProj.begin(), estProj.end());
        std::sort(targetProj.begin(), targetProj.end());

        // Compute L2 distance between sorted projections
        int minSize = std::min(estProj.size(), targetProj.size());
        for (int i = 0; i < minSize; ++i) {
            float diff = estProj[i] - targetProj[i];
            totalLoss += diff * diff;
        }
    }

    return totalLoss / numSlices;
}

std::vector<UT_Vector3> LossComputation::extractStrandFeatures(
    const StrandSet& strands)
{
    // Extract strand segments as features for Wasserstein distance
    // Features: differences between consecutive vertices
    std::vector<UT_Vector3> features;

    for (int i = 0; i < strands.getStrandCount(); ++i) {
        const Strand& strand = strands.getStrand(i);

        // Extract segments (edge vectors)
        for (size_t j = 0; j < strand.positions.size() - 1; ++j) {
            UT_Vector3 segment = strand.positions[j + 1] - strand.positions[j];
            features.push_back(segment);
        }
    }

    return features;
}

// ============================================================================
// Determinantal Point Process Loss (Section 5.5)
// For measuring strand correlations (clump, scale operators)
// ============================================================================

float LossComputation::computeDPPLoss(
    const StrandSet& estimatedStrands,
    const StrandSet& targetStrands,
    const std::string& operatorType)
{
    // Section 5.5: Strand Correlation Operator Optimization
    // DPP_L_ψ(x, x̂) = (log(det[ψ(x_a, x_b)]) - log(det[ψ(x̂_a, x̂_b)]))²
    //
    // DPP (Determinantal Point Process) theory:
    // det(K) measures how "spread out" points are
    // Large det = points far apart (negative correlation)
    // Small det = points clustered (positive correlation)
    //
    // ψ = kernel measuring similarity between strand pairs
    // For clump: ψ_clump = exp(-CD(x_a, x_b) / σ) 
    //            CD = relative distance (strands converge towards tip)
    // For scale: ψ_scale = exp(-(l_a - l_b)² / σ)
    //            l = strand length (scale operator creates length variation)
    //
    // Used for: clump, scale operators

    if (estimatedStrands.getStrandCount() == 0 || targetStrands.getStrandCount() == 0) {
        return 1000.0f;
    }

    // Simplified: compare strand correlation metrics
    // Full implementation would compute actual kernel determinants

    float estCorrelation = computeStrandCorrelation(estimatedStrands, operatorType);
    float targetCorrelation = computeStrandCorrelation(targetStrands, operatorType);

    float diff = estCorrelation - targetCorrelation;
    return diff * diff;
}

float LossComputation::computeStrandCorrelation(
    const StrandSet& strands,
    const std::string& operatorType)
{
    // Compute correlation metric for strands
    // Based on operator type (clump vs scale)

    if (strands.getStrandCount() < 2) {
        return 0.0f;
    }

    float correlation = 0.0f;
    int count = 0;

    // Sample neighboring strands (full matrix would be O(n²))
    int sampleSize = std::min(100, strands.getStrandCount() / 10);

    for (int i = 0; i < sampleSize && i < strands.getStrandCount(); ++i) {
        const Strand& strand1 = strands.getStrand(i);

        if (i + 1 < strands.getStrandCount()) {
            const Strand& strand2 = strands.getStrand(i + 1);

            if (operatorType == "clump") {
                // Clumpiness: how close strands are (Euclidean distance)
                // Clump operator pushes strands towards guide
                float dist = (strand1.positions[0] - strand2.positions[0]).length();
                correlation += 1.0f / (1.0f + dist);  // Inverse distance metric
            }
            else if (operatorType == "scale") {
                // Scale variation: length differences
                // Scale operator randomly scales strands
                float lenDiff = std::abs(strand1.arcLength - strand2.arcLength);
                correlation += lenDiff;
            }

            count++;
        }
    }

    return (count > 0) ? correlation / count : 0.0f;
}