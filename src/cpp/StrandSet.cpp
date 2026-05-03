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

void StrandSet::setDeformedAsPos() {
    for (Strand& s : strands) {
        s.deformedPositions.resize(s.positions.size());
        s.deformedPositions = s.positions;
    }
}

void StrandSet::applyScale(const GuideSet& guides, const float scaleFactor)
{
    std::random_device rd{};
    std::mt19937 gen{ rd() };
    std::normal_distribution<float> d{ 0.0f, 1.0f };

    for (Strand& s : strands) {
        s.deformedPositions = s.positions;

        float u = std::clamp(d(gen), -1.0f, 1.0f);
        float eta = 1 + u * scaleFactor;
        UT_Vector3 root = s.getRoot();

        for (int i = 0; i < s.positions.size(); ++i) {
            s.deformedPositions.at(i) = root + eta * (s.positions.at(i) - root);
        }
    }
}

void StrandSet::applyClump(const GuideSet& guides, const float clumpProfile)
{
    for (Strand& s : strands) {
        Strand guide = guides.getGuide(s.clusterID);

        // set root position
        std::vector<UT_Vector3> pos;
        pos.push_back(s.getRoot());

        // recompute arc length from deformed positions
        float length = 0.0f;
        for (int i = 1; i < s.deformedPositions.size(); ++i)
            length += (s.deformedPositions.at(i) - s.deformedPositions.at(i - 1)).length();

        float dist = 0;
        for (int i = 1; i < s.positions.size(); ++i) {
            // length of i-th segment
            float segmentLength = (s.deformedPositions.at(i) - s.deformedPositions.at(i-1)).length();
            dist += segmentLength;

            float normDist = dist / length;
            float decayAmt = 1.0f - exp(-clumpProfile * normDist);

            UT_Vector3 position = (1.0f - decayAmt) * s.deformedPositions.at(i) + decayAmt * guide.positions.at(i);
            pos.push_back(position);
        }

        s.deformedPositions = pos;
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
