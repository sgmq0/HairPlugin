#ifndef EXPORT_MANAGER_H
#define EXPORT_MANAGER_H

#include "StrandSet.h"
#include "GuideSet.h"
#include <string>

class ExportManager
{
public:
    // Export guides to OBJ file
    static bool exportGuides(
        const GuideSet& guides,
        const std::string& filepath);

    // Export synthesized strands to OBJ file (full density)
    static bool exportSynthesizedHair(
        const StrandSet& strands,
        const std::string& filepath);

private:
    // Helper to write OBJ polylines
    static bool writeOBJ(
        const std::string& filepath,
        const std::vector<std::vector<UT_Vector3>>& polylines);
};

#endif
