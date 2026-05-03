#include "StrandSet.h"

#include <algorithm>
#include <random>

StrandSet::StrandSet()
    : boundsValid(false)
{
}

StrandSet::~StrandSet()
{
}

void StrandSet::addStrand(const Strand& strand)
{
    if (strand.positions.empty())
        return;

    strands.push_back(strand);
    boundsValid = false;
}

Strand& StrandSet::getStrand(int index)
{
    return strands[index];
}

const Strand& StrandSet::getStrand(int index) const
{
    return strands[index];
}

void StrandSet::applyScale(const GuideSet& guides, const ClumpParams& params)
{
    std::random_device rd{};
    std::mt19937 gen{ rd() };
    std::normal_distribution<float> d{ 0.0f, 1.0f };

    for (Strand& s : strands) {
        s.deformedPositions = s.positions;
        s.applyScale(d(gen), params.scaleFactor);
    }
}

int StrandSet::getStrandCount() const
{
    return static_cast<int>(strands.size());
}

UT_BoundingBox StrandSet::getBounds() const
{
    if (!boundsValid)
    {
        cachedBounds.initBounds();

        for (const auto& strand : strands)
        {
            for (const auto& pos : strand.positions)
            {
                cachedBounds.enlargeBounds(pos);
            }
        }

        boundsValid = true;
    }

    return cachedBounds;
}

bool StrandSet::validate() const
{
    // Check strand count
    if (strands.size() < 10)
        return false;

    // Check each strand
    for (const auto& strand : strands)
    {
        // Need at least 2 points per strand
        if (strand.positions.size() < 2)
            return false;

        // Check for NaN or infinite values
        for (const auto& pos : strand.positions)
        {
            if (!std::isfinite(pos.x()) || !std::isfinite(pos.y()) || !std::isfinite(pos.z()))
                return false;
        }

        // Check arc length is valid
        if (strand.arcLength < 0.0f || !std::isfinite(strand.arcLength))
            return false;
    }

    return true;
}

void StrandSet::normalizeArchLengths()
{
    for (auto& strand : strands)
    {
        if (strand.arcLength < 1e-6f)
        {
            // Recompute arc length
            strand.arcLength = 0.0f;

            for (size_t i = 1; i < strand.positions.size(); ++i)
            {
                float segLen = (strand.positions[i] - strand.positions[i - 1]).length();
                strand.arcLength += segLen;
            }
        }
    }
}

void StrandSet::clear() {
    strands.clear();
}
