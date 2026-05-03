#ifndef STRANDSET_H
#define STRANDSET_H

#include "Strand.h"
#include <UT/UT_BoundingBox.h>
#include <vector>
#include "ClumpOperator.h"
#include "GuideSet.h"

class StrandSet
{
public:
    StrandSet();
    ~StrandSet();

    void addStrand(const Strand& strand);
    Strand& getStrand(int index);
    const Strand& getStrand(int index) const;

    // operators
    void setDeformedAsPos();
    void applyScale(const GuideSet& guides, const float scaleFactor);
    void applyClump(const GuideSet& guides, const float clumpProfile);

    int getStrandCount() const;
    UT_BoundingBox getBounds() const;
    bool validate() const;
    void normalizeArchLengths();
    void clear();

private:
    std::vector<Strand> strands;
    mutable UT_BoundingBox cachedBounds;
    mutable bool boundsValid;
};

#endif