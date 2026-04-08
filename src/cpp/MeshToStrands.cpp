#include "MeshToStrands.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <set>
#include <GEO/GEO_PrimPoly.h>

std::string MeshToStrands::lastErrorMessage = "";

StrandSet MeshToStrands::loadHairMeshFromOBJ(const std::string& filepath)
{
    StrandSet result;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        lastErrorMessage = "Failed to open OBJ file: " + filepath;
        return result;
    }

    try
    {
        std::vector<UT_Vector3> vertices;
        std::vector<std::vector<int>> faces;
        std::string line;

        // Parse OBJ file
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v")
            {
                // Vertex position
                float x, y, z;
                if (iss >> x >> y >> z)
                {
                    vertices.push_back(UT_Vector3(x, y, z));
                }
            }
            else if (prefix == "f")
            {
                // Face (can be triangles or quads)
                std::vector<int> faceIndices;
                std::string vertexData;

                while (iss >> vertexData)
                {
                    // Parse vertex reference (can be v, v/vt, v/vt/vn, v//vn)
                    std::istringstream vertexStream(vertexData);
                    int vertexIndex;
                    char slash;

                    if (vertexStream >> vertexIndex)
                    {
                        // OBJ uses 1-based indexing
                        faceIndices.push_back(vertexIndex - 1);
                    }
                }

                if (faceIndices.size() >= 3)
                {
                    faces.push_back(faceIndices);
                }
            }
        }

        file.close();

        if (vertices.empty())
        {
            lastErrorMessage = "No vertices found in OBJ file";
            return result;
        }

        if (faces.empty())
        {
            lastErrorMessage = "No faces found in OBJ file";
            return result;
        }

        // Extract curves from mesh
        std::vector<Strand> strands = extractCurvesFromMesh(vertices, faces);

        // Add all strands to result
        for (const auto& strand : strands)
        {
            if (strand.positions.size() >= 2)
            {
                result.addStrand(strand);
            }
        }

        if (result.getStrandCount() == 0)
        {
            lastErrorMessage = "Failed to extract any strands from mesh";
            return result;
        }

        lastErrorMessage = "";
    }
    catch (const std::exception& e)
    {
        lastErrorMessage = std::string("Exception loading OBJ: ") + e.what();
        return StrandSet();
    }

    return result;
}

StrandSet MeshToStrands::loadHairMeshFromHoudini(const GU_Detail* gdp)
{
    StrandSet result;

    if (!gdp)
    {
        lastErrorMessage = "Input geometry is null";
        return result;
    }

    try
    {
        // Extract vertices from Houdini geometry
        std::vector<UT_Vector3> vertices;
        std::vector<std::vector<int>> faces;

        // Get all points
        for (GA_Iterator it(gdp->getPointRange()); !it.atEnd(); ++it)
        {
            UT_Vector3 pos = gdp->getPos3(*it);
            vertices.push_back(pos);
        }

        // Get all primitives (faces/curves)
        for (GA_Iterator it(gdp->getPrimitiveRange()); !it.atEnd(); ++it)
        {
            const GA_Primitive* prim = gdp->getPrimitive(*it);

            if (prim->getTypeId() == GEO_PRIMPOLY)
            {
                const GEO_PrimPoly* poly = UTverify_cast<const GEO_PrimPoly*>(prim);
                std::vector<int> faceIndices;

                for (int i = 0; i < poly->getVertexCount(); ++i)
                {
                    GA_Offset ptoff = poly->getPointOffset(i);
                    GA_Index idx = gdp->pointIndex(ptoff);
                    faceIndices.push_back((int)idx);
                }

                if (faceIndices.size() >= 2)
                {
                    faces.push_back(faceIndices);
                }
            }
        }

        if (vertices.empty())
        {
            lastErrorMessage = "No vertices in input geometry";
            return result;
        }

        // Extract curves
        std::vector<Strand> strands = extractCurvesFromMesh(vertices, faces);

        // Add to result
        for (const auto& strand : strands)
        {
            if (strand.positions.size() >= 2)
            {
                result.addStrand(strand);
            }
        }

        lastErrorMessage = "";
    }
    catch (const std::exception& e)
    {
        lastErrorMessage = std::string("Exception loading from Houdini: ") + e.what();
        return StrandSet();
    }

    return result;
}

std::vector<Strand> MeshToStrands::extractCurvesFromMesh(
    const std::vector<UT_Vector3>& vertices,
    const std::vector<std::vector<int>>& faces)
{
    std::vector<Strand> strands;

    if (vertices.empty() || faces.empty())
        return strands;

    // Extract edges from faces
    std::vector<std::pair<int, int>> edges = extractEdgesFromFaces(faces);

    // For each edge, create a strand
    std::set<std::pair<int, int>> processedEdges;

    for (const auto& edge : edges)
    {
        int a = edge.first;
        int b = edge.second;

        // Avoid duplicate edges
        if (processedEdges.count({ a, b }) || processedEdges.count({ b, a }))
            continue;

        processedEdges.insert({ a, b });

        // Create strand from edge
        Strand strand;
        strand.positions.push_back(vertices[a]);
        strand.positions.push_back(vertices[b]);
        strand.radius.push_back(1.0f);
        strand.radius.push_back(1.0f);

        // Compute arc length
        float segLen = (vertices[b] - vertices[a]).length();
        strand.arcLength = segLen;

        if (strand.arcLength > 1e-6f)
        {
            strands.push_back(strand);
        }
    }

    return strands;
}

std::vector<std::pair<int, int>> MeshToStrands::extractEdgesFromFaces(
    const std::vector<std::vector<int>>& faces)
{
    std::vector<std::pair<int, int>> edges;
    std::set<std::pair<int, int>> uniqueEdges;

    for (const auto& face : faces)
    {
        // Each face contributes edges between consecutive vertices
        for (size_t i = 0; i < face.size(); ++i)
        {
            int v1 = face[i];
            int v2 = face[(i + 1) % face.size()];

            // Normalize edge representation (smaller index first)
            int a = std::min(v1, v2);
            int b = std::max(v1, v2);

            if (a != b && !uniqueEdges.count({ a, b }))
            {
                uniqueEdges.insert({ a, b });
                edges.push_back({ a, b });
            }
        }
    }

    return edges;
}

std::vector<UT_Vector3> MeshToStrands::traceCurveOnMesh(
    const std::vector<UT_Vector3>& vertices,
    int startVertexIndex,
    const std::vector<std::vector<int>>& faces)
{
    // For now, simple implementation: just return start vertex
    // Can be enhanced to trace along surface curves
    std::vector<UT_Vector3> curve;
    curve.push_back(vertices[startVertexIndex]);
    return curve;
}

std::vector<UT_Vector3> MeshToStrands::simplifyCurve(
    const std::vector<UT_Vector3>& curve,
    float tolerance)
{
    if (curve.size() <= 2)
        return curve;

    std::vector<UT_Vector3> simplified;
    simplified.push_back(curve.front());

    for (size_t i = 1; i < curve.size() - 1; ++i)
    {
        // Check if point is necessary (not collinear with neighbors)
        UT_Vector3 v1 = curve[i] - curve[i - 1];
        UT_Vector3 v2 = curve[i + 1] - curve[i];

        float len1 = v1.length();
        float len2 = v2.length();

        if (len1 > 1e-6f && len2 > 1e-6f)
        {
            v1 /= len1;
            v2 /= len2;

            float dot = v1.dot(v2);
            float angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));

            if (angle > tolerance)
            {
                simplified.push_back(curve[i]);
            }
        }
    }

    simplified.push_back(curve.back());
    return simplified;
}

std::string MeshToStrands::getLastError()
{
    return lastErrorMessage;
}