#ifndef GUIDESET_H
#define GUIDESET_H

#include "Strand.h"
#include <SOP/SOP_Node.h>
#include <UT/UT_BoundingBox.h>
#include <vector>

class GuideSet
{
public:
    GuideSet();
    ~GuideSet();

    void extractFromStrands(const std::vector<Strand>& strands, int numGuides);

    Strand& getGuide(int index);
    const Strand& getGuide(int index) const;

    int getStrandCluster(int strandIndex) const;

    int getGuideCount() const;
    int getTotalStrandCount() const;
    UT_BoundingBox getBounds() const;

    void computeUVLocations(GU_Detail* gdp);

private:
    std::vector<Strand> guides;
    std::vector<int> clusterMap;
    int totalStrands;
};

#endif