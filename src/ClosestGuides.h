#ifndef CLOSEST_GUIDES_H
#define CLOSEST_GUIDES_H

#include "GuideSet.h"
#include <vector>

class ClosestGuides
{
public:
    ClosestGuides();

    // Build spatial index from guides
    void fillKDTree(GuideSet& guides);

private:
    // Placeholder for KDTree - not implemented yet
    // Can use nanoflann in future if needed
};

#endif