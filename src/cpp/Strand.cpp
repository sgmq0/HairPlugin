#include "Strand.h"
#include "GuideSet.h"
#include <cmath>
#include <algorithm>
#include <GU/GU_RayIntersect.h>
#include <GEO/GEO_PrimPoly.h>
#include <GA/GA_Handle.h>

class GuideSet;

Strand::Strand()
    : arcLength(0.0f), clusterID(-1), reconstructionError(0.0f)
{
}

Strand::~Strand()
{
}

void Strand::normalize()
{
    if (arcLength < 1e-6f) return;

    float scale = 1.0f / arcLength;
    for (auto& pos : positions)
        pos *= scale;
    for (auto& r : radius)
        r *= scale;
    arcLength = 1.0f;
}

void Strand::resample(int numPoints)
{
    if (numPoints <= 1 || positions.size() < 2)
        return;

    std::vector<UT_Vector3> newPositions;
    newPositions.reserve(numPoints);

    std::vector<float> newRadius;
    newRadius.reserve(numPoints);

    // Compute cumulative arc lengths
    std::vector<float> cumulativeLength;
    cumulativeLength.push_back(0.0f);

    for (size_t i = 1; i < positions.size(); ++i)
    {
        float segLen = (positions[i] - positions[i - 1]).length();
        cumulativeLength.push_back(cumulativeLength.back() + segLen);
    }

    float totalLen = cumulativeLength.back();
    if (totalLen < 1e-6f)
        return;

    // Resample at uniform intervals
    for (int i = 0; i < numPoints; ++i)
    {
        float t = (numPoints > 1) ? static_cast<float>(i) / (numPoints - 1) : 0.5f;
        float targetLen = t * totalLen;

        // Binary search for segment
        auto it = std::lower_bound(cumulativeLength.begin(), cumulativeLength.end(), targetLen);
        int idx = std::max(0, static_cast<int>(std::distance(cumulativeLength.begin(), it)) - 1);
        idx = std::min(idx, static_cast<int>(positions.size()) - 2);

        // Linear interpolation
        float segLen = cumulativeLength[idx + 1] - cumulativeLength[idx];
        float localT = (segLen > 1e-6f) ? (targetLen - cumulativeLength[idx]) / segLen : 0.0f;

        UT_Vector3 interpPos = positions[idx] + localT * (positions[idx + 1] - positions[idx]);
        float interpRadius = radius[idx] + localT * (radius[idx + 1] - radius[idx]);

        newPositions.push_back(interpPos);
        newRadius.push_back(interpRadius);
    }

    positions = newPositions;
    radius = newRadius;
}

UT_Vector3 Strand::getTipDirection() const
{
    if (positions.size() < 2) return UT_Vector3(0, 1, 0);

    UT_Vector3 direction = positions.back() - positions.front();
    float len = direction.length();

    if (len > 1e-6f)
        direction /= len;
    else
        direction = UT_Vector3(0, 1, 0);

    return direction;
}

float Strand::computeAverageRadius() const
{
    if (radius.empty())
        return 1.0f;

    float sum = 0.0f;
    for (float r : radius)
        sum += r;

    return sum / radius.size();
}

float Strand::computeAverageCurvature() const
{
    if (positions.size() < 3)
        return 0.0f;

    float totalCurvature = 0.0f;
    int count = 0;

    for (size_t i = 1; i < positions.size() - 1; ++i)
    {
        UT_Vector3 v1 = positions[i] - positions[i - 1];
        UT_Vector3 v2 = positions[i + 1] - positions[i];

        float len1 = v1.length();
        float len2 = v2.length();

        if (len1 > 1e-6f && len2 > 1e-6f)
        {
            v1 /= len1;
            v2 /= len2;

            float curvature = std::acos(std::max(-1.0f, std::min(1.0f, v1.dot(v2))));
            totalCurvature += curvature;
            count++;
        }
    }

    return (count > 0) ? totalCurvature / count : 0.0f;
}

void Strand::computeUVLocation(GU_Detail* gdp, GU_MinInfo* minInfo)
{
    if (!minInfo->prim) {
        root_UV = UT_Vector2(0, 0);
        return;
    }

    // get the primitive and barycentric coords of closest point
    const GEO_Primitive* prim = minInfo->prim;
    float u = minInfo->u1; // barycentric u
    float v = minInfo->v1; // barycentric v

    // look up UV attribute on the mesh
    GA_ROHandleV3 uvHandle(gdp->findTextureAttribute(GA_ATTRIB_VERTEX));
    if (!uvHandle.isValid())
    {
        root_UV = UT_Vector2(0, 0);
        return;
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

    root_UV = UT_Vector2(uvInterp.x(), uvInterp.y());
}

void Strand::computeGuideWeights(GuideSet* guideSet) {
    for (size_t j = 0; j < guides.size(); j++) {
        float dist = guides.at(j).first;
        int idx = guides.at(j).second;

        UT_Vector2 T_x = root_UV;
        UT_Vector2 T_g = guideSet->getGuide(idx).root_UV;

        float sigma = 2.0; // size of guide's region of influence. not sure where to get this value from

        float d = T_x.distance(T_g);
        float expo = -(d * d) / (sigma * sigma);
        float weight = exp(expo);

        std::pair<float, int> pair = std::pair<float, int>(weight, idx);
        guideWeights.push_back(pair);
    }
}

UT_Vector3 Strand::getRoot() const
{
    return positions.front();
}
