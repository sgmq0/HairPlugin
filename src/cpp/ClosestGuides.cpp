#include "ClosestGuides.h"

ClosestGuides::ClosestGuides()
{
}

void ClosestGuides::fillKDTree(GuideSet& guides)
{
    int N = guides.getGuideCount();
    pc.pts.resize(N);
    for (size_t i = 0; i < N; i++)
    {
        Strand g = guides.getGuide(i);
        UT_Vector3 guide_root = g.getRoot();
        pc.pts[i].assign(guide_root.data());
    }
}
