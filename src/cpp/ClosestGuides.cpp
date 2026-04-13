#include "ClosestGuides.h"

ClosestGuides::ClosestGuides()
{
}

void ClosestGuides::fillKDTree(GuideSet& guides)
{
    nodes.clear();
    root = -1;

    if (guides.getGuideCount() == 0) 
        return;

    // build list of (position, index) pairs from guide UVs
    std::vector<std::pair<UT_Vector2, int>> points;
    for (int i = 0; i < guides.getGuideCount(); i++) {
        UT_Vector2 uv = guides.getGuide(i).root_UV;
        points.push_back({ uv, i });
    }

    root = build(points, 0, points.size(), 0);
}

std::vector<std::pair<float, int>> ClosestGuides::findNNearest(const UT_Vector2& uv, int n)
{
    std::vector<std::pair<float, int>> indices;
    if (root < 0 || n <= 0) return indices;

    UT_Vector2 query(uv.x(), uv.y());

    // keeps track of the N best candidates found so far
    std::priority_queue<std::pair<float, int>> heap;

    searchNN(root, query, n, heap);

    // extract results from heap (closest first)
    while (!heap.empty())
    {
        indices.push_back(heap.top());
        heap.pop();
    }
    std::reverse(indices.begin(), indices.end());

    return indices;
}

int ClosestGuides::build(std::vector<std::pair<UT_Vector2, int>>& points, int start, int end, int depth)
{
    if (start >= end) return -1;

    // split along alternating axes
    int axis = depth % 2;
    int mid = (start + end) / 2;

    // partial sort so median element is in the middle
    std::nth_element(
        points.begin() + start,
        points.begin() + mid,
        points.begin() + end,
        [axis](const std::pair<UT_Vector2, int>& a,
            const std::pair<UT_Vector2, int>& b)
        {
            return a.first[axis] < b.first[axis];
        }
    );

    // create node for median point
    KDNode node;
    node.UV_pos = points[mid].first;
    node.guideIndex = points[mid].second;
    node.axis = axis;
    node.left = -1;
    node.right = -1;

    int nodeIdx = (int)nodes.size();
    nodes.push_back(node);

    // recurse into left and right subtrees
    int leftIdx = build(points, start, mid, depth + 1);
    int rightIdx = build(points, mid + 1, end, depth + 1);

    nodes[nodeIdx].left = leftIdx;
    nodes[nodeIdx].right = rightIdx;

    return nodeIdx;
}

void ClosestGuides::searchNN(int nodeIdx, const UT_Vector2& query, int n, std::priority_queue<std::pair<float, int>>& heap)
{
    if (nodeIdx < 0) return;

    const KDNode& node = nodes[nodeIdx];

    // compute squared distance from query to this node
    float dx = query[0] - node.UV_pos[0];
    float dy = query[1] - node.UV_pos[1];
    float dist = dx * dx + dy * dy;

    // add to heap if we have fewer than n results,
    // or if this point is closer than the current worst
    if ((int)heap.size() < n)
    {
        heap.push({ dist, node.guideIndex });
    }
    else if (dist < heap.top().first)
    {
        heap.pop();
        heap.push({ dist, node.guideIndex });
    }

    // recurse into the closer half first
    int axis = node.axis;
    float axisDiff = query[axis] - node.UV_pos[axis];
    int near = axisDiff < 0 ? node.left : node.right;
    int far = axisDiff < 0 ? node.right : node.left;

    searchNN(near, query, n, heap);

    float worstDist = heap.top().first;
    if ((int)heap.size() < n || axisDiff * axisDiff < worstDist)
        searchNN(far, query, n, heap);
}
