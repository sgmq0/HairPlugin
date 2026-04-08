#include "ClusterVisualization.h"
#include <cmath>
#include <algorithm>

std::vector<UT_Vector3> ClusterVisualization::generateClusterColors(int numClusters)
{
    std::vector<UT_Vector3> colors;

    for (int i = 0; i < numClusters; ++i)
    {
        // Evenly space hues around color wheel
        float hue = static_cast<float>(i) / std::max(1, numClusters);
        UT_Vector3 color = hsvToRgb(hue, 0.8f, 0.9f);  // High saturation and brightness
        colors.push_back(color);
    }

    return colors;
}

UT_Vector3 ClusterVisualization::getClusterColor(int clusterID, int totalClusters)
{
    if (clusterID < 0 || totalClusters <= 0)
        return UT_Vector3(0.5f, 0.5f, 0.5f);  // Gray for invalid cluster

    float hue = static_cast<float>(clusterID) / totalClusters;
    return hsvToRgb(hue, 0.8f, 0.9f);
}

float ClusterVisualization::getHighlightFactor(
    int strandCluster,
    int selectedCluster,
    bool hasSelection)
{
    if (!hasSelection)
        return 1.0f;  // No selection, show everything fully

    if (selectedCluster < 0)
        return 1.0f;  // Invalid selection, show everything

    if (strandCluster == selectedCluster)
        return 1.0f;  // This strand is in selected cluster, highlight fully

    return 0.2f;  // Dim other strands (20% brightness)
}

void ClusterVisualization::assignClustersToStrands(
    StrandSet& strands,
    const GuideSet& guides)
{
    // For each strand, find which cluster it belongs to
    // This is already done by K-means, but we can verify here

    for (int i = 0; i < strands.getStrandCount(); ++i)
    {
        Strand& strand = strands.getStrand(i);
        int cluster = guides.getStrandCluster(i);
        strand.clusterID = cluster;
    }
}

UT_Vector3 ClusterVisualization::hsvToRgb(float h, float s, float v)
{
    // Clamp values to [0, 1]
    h = std::fmod(h, 1.0f);
    if (h < 0) h += 1.0f;
    s = std::max(0.0f, std::min(1.0f, s));
    v = std::max(0.0f, std::min(1.0f, v));

    float c = v * s;  // Chroma
    float hPrime = h * 6.0f;
    float x = c * (1.0f - std::abs(std::fmod(hPrime, 2.0f) - 1.0f));

    float r = 0, g = 0, b = 0;

    if (hPrime < 1.0f)
    {
        r = c;
        g = x;
    }
    else if (hPrime < 2.0f)
    {
        r = x;
        g = c;
    }
    else if (hPrime < 3.0f)
    {
        g = c;
        b = x;
    }
    else if (hPrime < 4.0f)
    {
        g = x;
        b = c;
    }
    else if (hPrime < 5.0f)
    {
        r = x;
        b = c;
    }
    else
    {
        r = c;
        b = x;
    }

    float m = v - c;
    return UT_Vector3(r + m, g + m, b + m);
}
