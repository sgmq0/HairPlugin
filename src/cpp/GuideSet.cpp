#include "GuideSet.h"
#include "GuideSmoothing.h"
#include <algorithm>
#include <random>
#include <GU/GU_RayIntersect.h>
#include <GEO/GEO_PrimPoly.h>
#include <GA/GA_Handle.h>

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

    // random engine
    std::random_device rd{};
    std::mt19937 gen{ rd() };
    std::normal_distribution<float> d{ 0.0f, 1.0f };
    std::uniform_real_distribution<float> u{ 0.0f, 1.0f };

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

        // set random values
        smoothedGuide.bendRand = d(gen);
        smoothedGuide.curlNormal = d(gen);
        smoothedGuide.curlUniform = u(gen);

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

void GuideSet::computeUVLocations(GU_Detail* gdp)
{
    const GA_PrimitiveGroup* scalpGroup = gdp->findPrimitiveGroup("scalp");
    GU_RayIntersect ray(gdp, scalpGroup);

    for (Strand& s : guides) {
        UT_Vector3 root = s.getRoot();

        // find closest point on mesh surface
        GU_MinInfo minInfo;
        ray.minimumPoint(root, minInfo);

        if (!minInfo.prim) {
            s.root_UV = UT_Vector2(0, 0);
            continue;
        }

        // s.root_UV = UT_Vector2(1.0, 0.0);

        // get the primitive and barycentric coords of closest point
        const GEO_Primitive* prim = minInfo.prim;
        float u = minInfo.u1; // barycentric u
        float v = minInfo.v1; // barycentric v

        // look up UV attribute on the mesh
        GA_ROHandleV3 uvHandle(gdp->findTextureAttribute(GA_ATTRIB_VERTEX));
        if (!uvHandle.isValid())
        {
            s.root_UV = UT_Vector2(0, 0);
            continue;
        }

        // interpolate UV across the primitive using barycentric coords
        // for a quad/tri, walk the vertices and blend
        int numVerts = prim->getVertexCount();
        UT_Vector3 uvInterp(0, 0, 0);

        if (numVerts == 3)
        {
            // triangle: standard barycentric interpolation
            UT_Vector3 uv0 = uvHandle.get(prim->getVertexOffset(0));
            UT_Vector3 uv1 = uvHandle.get(prim->getVertexOffset(1));
            UT_Vector3 uv2 = uvHandle.get(prim->getVertexOffset(2));
            uvInterp = uv0 * (1 - u - v) + uv1 * u + uv2 * v;
        }
        else if (numVerts == 4)
        {
            // quad: bilinear interpolation
            UT_Vector3 uv0 = uvHandle.get(prim->getVertexOffset(0));
            UT_Vector3 uv1 = uvHandle.get(prim->getVertexOffset(1));
            UT_Vector3 uv2 = uvHandle.get(prim->getVertexOffset(2));
            UT_Vector3 uv3 = uvHandle.get(prim->getVertexOffset(3));
            uvInterp = uv0 * (1 - u) * (1 - v)
                + uv1 * u * (1 - v)
                + uv2 * u * v
                + uv3 * (1 - u) * v;
        }

        s.root_UV = UT_Vector2(uvInterp.x(), uvInterp.y());
    }
}

