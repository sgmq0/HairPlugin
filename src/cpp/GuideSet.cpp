#include "GuideSet.h"
#include "GuideSmoothing.h"
#include <algorithm>

GuideSet::GuideSet() : totalStrands(0)
{
}

GuideSet::~GuideSet()
{
}

void GuideSet::extractFromStrands(
    const std::vector<Strand>& strands, int numGuides)
{
    guides.clear();
    clusterMap.clear();
    totalStrands = strands.size();

    if (strands.empty() || numGuides <= 0)
        return;

    // Group strands into clusters
    clusterMap.resize(strands.size());

    std::vector<std::vector<int>> clusters(numGuides);
    for (int i = 0; i < (int)strands.size(); ++i)
    {
        int cluster = (i * numGuides) / strands.size();
        cluster = std::min(cluster, numGuides - 1);
        clusters[cluster].push_back(i);
        clusterMap[i] = cluster;
    }

    // Extract representative guide per cluster
    guides.reserve(numGuides);

    for (int k = 0; k < numGuides; ++k)
    {
        if (clusters[k].empty())
            continue;

        // Average positions of strands in cluster
        int numStrands = clusters[k].size();
        int maxPoints = 0;

        for (int idx : clusters[k])
            maxPoints = std::max(maxPoints, (int)strands[idx].positions.size());

        Strand avgStrand;
        avgStrand.positions.resize(maxPoints, UT_Vector3(0, 0, 0));
        avgStrand.radius.resize(maxPoints, 0.0f);
        avgStrand.clusterID = k;

        // Compute average positions
        for (int idx : clusters[k])
        {
            const Strand& strand = strands[idx];
            for (size_t i = 0; i < strand.positions.size(); ++i)
            {
                avgStrand.positions[i] += strand.positions[i];
                avgStrand.radius[i] += strand.radius[i];
            }
        }

        // Divide by count
        for (int i = 0; i < maxPoints; ++i)
        {
            avgStrand.positions[i] /= numStrands;
            avgStrand.radius[i] /= numStrands;
        }

        // Compute arc length
        avgStrand.arcLength = 0.0f;
        for (size_t i = 1; i < avgStrand.positions.size(); ++i)
        {
            float segLen = (avgStrand.positions[i] -
                avgStrand.positions[i - 1]).length();
            avgStrand.arcLength += segLen;
        }

        // Smooth the guide curve
        Strand smoothedGuide = GuideSmoothing::smoothGuideCurve(avgStrand, 10);

        guides.push_back(smoothedGuide);
    }
}

Strand& GuideSet::getGuide(int index)
{
    return guides[index];
}

const Strand& GuideSet::getGuide(int index) const
{
    return guides[index];
}

int GuideSet::getStrandCluster(int strandIndex) const
{
    if (strandIndex < 0 || strandIndex >= (int)clusterMap.size())
        return -1;
    return clusterMap[strandIndex];
}

int GuideSet::getGuideCount() const
{
    return static_cast<int>(guides.size());
}

int GuideSet::getTotalStrandCount() const
{
    return totalStrands;
}

UT_BoundingBox GuideSet::getBounds() const
{
    UT_BoundingBox bounds;
    bounds.initBounds();

    for (const auto& guide : guides)
        for (const auto& pos : guide.positions)
            bounds.enlargeBounds(pos);

    return bounds;
}