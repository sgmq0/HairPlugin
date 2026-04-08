#ifndef MESH_TO_STRANDS_H
#define MESH_TO_STRANDS_H

#include "StrandSet.h"
#include <string>
#include <GU/GU_Detail.h>

class MeshToStrands
{
public:
    // Load hair mesh from OBJ file and convert to strands
    // Each curve/edge in the mesh becomes a strand
    // Returns StrandSet with all extracted strands
    static StrandSet loadHairMeshFromOBJ(const std::string& filepath);

    // Load hair mesh from Houdini geometry
    // Extracts curves as strands
    static StrandSet loadHairMeshFromHoudini(const GU_Detail* gdp);

    // Get last error message if loading failed
    static std::string getLastError();

private:
    // Extract curves from mesh geometry
    // For each closed loop/curve, create a strand
    static std::vector<Strand> extractCurvesFromMesh(
        const std::vector<UT_Vector3>& vertices,
        const std::vector<std::vector<int>>& faces);

    // Trace a single curve on mesh surface
    // Returns vector of positions along the curve
    static std::vector<UT_Vector3> traceCurveOnMesh(
        const std::vector<UT_Vector3>& vertices,
        int startVertexIndex,
        const std::vector<std::vector<int>>& faces);

    // Find edges in mesh (curves along surface)
    static std::vector<std::pair<int, int>> extractEdgesFromFaces(
        const std::vector<std::vector<int>>& faces);

    // Simplify curve by removing redundant points
    static std::vector<UT_Vector3> simplifyCurve(
        const std::vector<UT_Vector3>& curve,
        float tolerance = 0.01f);

    static std::string lastErrorMessage;
};

#endif