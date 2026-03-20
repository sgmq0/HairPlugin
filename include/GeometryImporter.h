#ifndef GEOMETRY_IMPORTER_H
#define GEOMETRY_IMPORTER_H

#include <GU/GU_Detail.h>
#include <string>
#include "StrandSet.h"

class GeometryImporter
{
public:
    static StrandSet loadFromHoudiniGeometry(const GU_Detail* gdp);
    static StrandSet loadFromOBJFile(const std::string& filepath);
    static StrandSet loadFromCurveFile(const std::string& filepath);
    static bool validateGeometry(const GU_Detail* gdp);
    static std::string getLastError();

private:
    static std::string lastErrorMessage;
    static void extractCurvesFromDetail(const GU_Detail* gdp, StrandSet& outStrands);
    static Strand convertCurveToStrand(const GU_Detail* gdp, GA_Offset primoff);
};

#endif