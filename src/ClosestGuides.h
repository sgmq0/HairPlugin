#ifndef CLOSEST_GUIDES_H
#define CLOSEST_GUIDES_H

#include "GuideSet.h"
#include <UT/UT_KDTree.h>
#include <vector>
#include <queue>

struct KDNode
{
    UT_Vector2 UV_pos; // (u, v)
    int guideIndex;
    int left = -1;
    int right = -1;
    int axis = 0;
};

class ClosestGuides
{
public:
    ClosestGuides();

    // Build spatial index from guides
    void fillKDTree(GuideSet& guides);

    std::vector<std::pair<float, int>> findNNearest(const UT_Vector2& uv, int n);

    int getRoot() {
        return root;
    }

private:
    std::vector<KDNode> nodes;
    int root = -1;

    int build(std::vector<std::pair<UT_Vector2, int>>& points, int start, int end, int depth);
    void searchNN(int nodeIdx, const UT_Vector2& query, int n, std::priority_queue<std::pair<float, int>>& heap);
};

#endif