#ifndef CLUSTER_VISUALIZATION_H
#define CLUSTER_VISUALIZATION_H

#include "StrandSet.h"
#include "GuideSet.h"
#include <UT/UT_Vector3.h>
#include <vector>

class ClusterVisualization
{
public:
    // Generate rainbow colors for clusters
    // Each cluster gets a different color
    // Returns vector of colors (one per cluster)
    static std::vector<UT_Vector3> generateClusterColors(int numClusters);

    // Get color for a specific cluster
    static UT_Vector3 getClusterColor(int clusterID, int totalClusters);

    // Determine if strand should be highlighted
    // Returns brightness/opacity factor
    // 1.0 = full brightness (selected or no selection)
    // 0.3 = dimmed (not selected, but other strands are)
    static float getHighlightFactor(
        int strandCluster,
        int selectedCluster,
        bool hasSelection);

    // Update strand cluster assignments after K-means
    // This should be called after clustering
    static void assignClustersToStrands(
        StrandSet& strands,
        const GuideSet& guides);

private:
    // HSV to RGB conversion for rainbow colors
    static UT_Vector3 hsvToRgb(float h, float s, float v);
};

#endif
