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

void StrandSet::applyScale(const float scaleFactor)
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

        float length = s.arcLength;
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

float sigmoid(float x)
{
    return 1.0f / (1.0f + exp(-x));
}

void StrandSet::applyBend(const GuideSet& guides, const float bendAngle, const float bendStart)
{
    float bendAngleRadians = SYSdegToRad(bendAngle);

    for (Strand& s : strands) {
        Strand guide = guides.getGuide(s.clusterID);
        //float u = d(gen);
        float u = guide.bendRand;

        // find guide root direction
        UT_Vector3 guideRootDir = guide.positions.at(1) - guide.positions.at(0);
        guideRootDir.normalize();

        // find bend axis
        UT_Vector3 bendAxisAux = { 1.0f, 0.0f, 0.0f };
        if (abs(guideRootDir.y()) < 0.1f)
            bendAxisAux = { 0.0f, 1.0f, 0.0f };
        UT_Vector3 bendAxis = cross(guideRootDir, bendAxisAux);

        // construct axis-angle matrix
        UT_Matrix3F R1;
        R1.identity();
        R1.rotate(guideRootDir, M_PI * u);

        // rotate bend axis
        UT_Vector3 rotatedBendAxis = bendAxis * R1 / (bendAxis * R1).length();

        // set root position
        std::vector<UT_Vector3> pos;
        pos.push_back(s.getRoot());

        float length = s.arcLength;
        float dist = 0.0f;
        float lengthAfterBend = 0.0f;    // lprime
        float normDistanceFromBend = 0.0f;
        UT_Vector3 bendSegmentSum = { 0.0f, 0.0f, 0.0f };
        for (int i = 1; i < s.positions.size(); ++i) {
            // length of i-th segment
            float segmentLength = (s.deformedPositions.at(i) - s.deformedPositions.at(i - 1)).length();
            dist += segmentLength;
            float normDist = dist / length; // v_j

            // start smooth indicator
            float startSmooth = sigmoid(10 * (normDist - bendStart));

            // length after bend start
            lengthAfterBend += segmentLength * startSmooth;

            // norm. distance from bend start
            normDistanceFromBend += (segmentLength * startSmooth) / lengthAfterBend;

            // contruct r2 matrix
            UT_Matrix3F R2;
            R2.identity();
            R2.rotate(rotatedBendAxis, bendAngleRadians * normDistanceFromBend);

            // find bend segment
            UT_Vector3 bendSegment = (s.deformedPositions.at(i) - s.deformedPositions.at(i-1)) * R2;

            // sum segments to get vertices
            bendSegmentSum += bendSegment;
            pos.push_back(s.getRoot() + bendSegmentSum);
        }

        s.deformedPositions = pos;
    }
}

void StrandSet::applyCurl(
    const GuideSet& guides, 
    const float curlRadius, 
    const float curlFrequency, 
    const float curlRandomFrequency, 
    const float curlStart)
{
    for (Strand& s : strands) {
        Strand guide = guides.getGuide(s.clusterID);

        // find frequency
        float frequency = fmax(3 * (curlFrequency + curlRandomFrequency * guide.curlNormal), 0.0f);

        // set root position
        std::vector<UT_Vector3> pos;
        pos.push_back(s.getRoot());

        float length = s.arcLength;
        float freqSum = 0.0f;
        float distance = 0.0f;
        for (int j = 1; j < s.positions.size(); ++j) {
            float dist = (s.deformedPositions.at(j) - s.deformedPositions.at(j - 1)).length();
            distance += dist;
            float normDist = distance / length;
            
            // compute phase
            freqSum += dist * frequency;
            float phase = freqSum + 3.9f * guide.curlUniform;

            // increase radius towards tip
            float radiusPrime = curlRadius * (0.43f + 0.79f * normDist);

            // curl shape
            float curlShapeX = radiusPrime * sin(2.0f * M_PI * phase);
            float curlShapeY = radiusPrime * cos(2.0f * M_PI * phase);

            // guide segment direction
            UT_Vector3 direction = { 0.0f, 0.0f, 0.0f };
            if (j == s.positions.size() - 1) { // if this is the last point
                direction = (guide.positions.at(j) - guide.positions.at(j - 1));
            }
            else {
                direction = (guide.positions.at(j + 1) - guide.positions.at(j - 1));
            }
            direction.normalize();

            // normal
            UT_Vector3 normal = { 0.0f, 0.0f, 0.0f };
            if (abs(direction.x()) + abs(direction.y()) < 1e-4) {
                normal = { 1.0f, 0.0f, 0.0f };
            }
            else {
                normal = { direction.y(), -direction.x(), 0.0f };
                normal.normalize();
            }

            // binormal
            UT_Vector3 binormal = cross(direction, normal);

            // start smooth indicator
            float startSmooth = sigmoid(10.0f * (normDist - curlStart));

            // add curl shape
            UT_Vector3 deformedPos = s.deformedPositions.at(j) + startSmooth * (curlShapeX * normal + curlShapeY * binormal);
            pos.push_back(deformedPos);
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
