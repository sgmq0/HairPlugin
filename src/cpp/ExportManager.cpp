#include "ExportManager.h"
#include <fstream>
#include <sstream>

bool ExportManager::exportGuides(
    const GuideSet& guides,
    const std::string& filepath)
{
    std::vector<std::vector<UT_Vector3>> polylines;

    // Convert each guide to a polyline
    for (int i = 0; i < guides.getGuideCount(); ++i) {
        const Strand& guide = guides.getGuide(i);
        
        if (guide.positions.size() >= 2) {
            polylines.push_back(guide.positions);
        }
    }

    return writeOBJ(filepath, polylines);
}

bool ExportManager::exportSynthesizedHair(
    const StrandSet& strands,
    const std::string& filepath)
{
    std::vector<std::vector<UT_Vector3>> polylines;

    // Convert each strand to a polyline
    for (int i = 0; i < strands.getStrandCount(); ++i) {
        const Strand& strand = strands.getStrand(i);
        
        if (strand.positions.size() >= 2) {
            polylines.push_back(strand.positions);
        }
    }

    return writeOBJ(filepath, polylines);
}

bool ExportManager::writeOBJ(
    const std::string& filepath,
    const std::vector<std::vector<UT_Vector3>>& polylines)
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << "# Exported hair strands/guides\n";
    file << "# Total polylines: " << polylines.size() << "\n\n";

    int vertexCount = 0;

    // Write vertices and polyline definitions
    for (const auto& polyline : polylines) {
        // Write vertices
        for (const auto& pos : polyline) {
            file << "v " << pos.x() << " " << pos.y() << " " << pos.z() << "\n";
        }

        // Write polyline (l prefix for lines in OBJ)
        file << "l";
        for (int i = 0; i < (int)polyline.size(); ++i) {
            file << " " << (vertexCount + i + 1);  // OBJ uses 1-based indexing
        }
        file << "\n";

        vertexCount += (int)polyline.size();
    }

    file.close();
    return true;
}
