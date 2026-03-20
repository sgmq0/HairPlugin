#include "GeometryImporter.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string GeometryImporter::lastErrorMessage = "";

StrandSet GeometryImporter::loadFromHoudiniGeometry(const GU_Detail* gdp)
{
    StrandSet strands;

    if (!gdp)
    {
        lastErrorMessage = "Input geometry is null";
        return strands;
    }

    if (!validateGeometry(gdp))
    {
        lastErrorMessage = "Input geometry validation failed";
        return strands;
    }

    try
    {
        // For now, just return empty - will be replaced with actual curve extraction
        lastErrorMessage = "";
    }
    catch (const std::exception& e)
    {
        lastErrorMessage = std::string("Exception loading geometry: ") + e.what();
        return StrandSet();
    }

    return strands;
}

StrandSet GeometryImporter::loadFromOBJFile(const std::string& filepath)
{
    StrandSet strands;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        lastErrorMessage = "Failed to open OBJ file: " + filepath;
        return strands;
    }

    try
    {
        std::vector<UT_Vector3> vertices;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v")
            {
                float x, y, z;
                if (iss >> x >> y >> z)
                {
                    vertices.push_back(UT_Vector3(x, y, z));
                }
            }
        }

        file.close();

        if (vertices.size() < 10)
        {
            lastErrorMessage = "OBJ file has too few vertices";
            return strands;
        }

        Strand strand;
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            strand.positions.push_back(vertices[i]);
            strand.radius.push_back(1.0f);

            if (strand.positions.size() >= 20 && i % 25 == 0)
            {
                strand.arcLength = 0.0f;
                for (size_t j = 1; j < strand.positions.size(); ++j)
                {
                    float segLen = (strand.positions[j] - strand.positions[j - 1]).length();
                    strand.arcLength += segLen;
                }

                if (strand.arcLength > 1e-6f)
                {
                    strands.addStrand(strand);
                }

                strand.positions.clear();
                strand.radius.clear();
            }
        }

        if (!strand.positions.empty())
        {
            strand.arcLength = 0.0f;
            for (size_t j = 1; j < strand.positions.size(); ++j)
            {
                float segLen = (strand.positions[j] - strand.positions[j - 1]).length();
                strand.arcLength += segLen;
            }

            if (strand.arcLength > 1e-6f)
            {
                strands.addStrand(strand);
            }
        }

        lastErrorMessage = "";
    }
    catch (const std::exception& e)
    {
        lastErrorMessage = std::string("Exception loading OBJ: ") + e.what();
        return StrandSet();
    }

    return strands;
}

StrandSet GeometryImporter::loadFromCurveFile(const std::string& filepath)
{
    StrandSet strands;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        lastErrorMessage = "Failed to open curve file: " + filepath;
        return strands;
    }

    try
    {
        int numStrands = 0;
        if (!(file >> numStrands) || numStrands <= 0)
        {
            lastErrorMessage = "Invalid curve file format";
            return strands;
        }

        for (int s = 0; s < numStrands; ++s)
        {
            int numPoints = 0;
            if (!(file >> numPoints) || numPoints < 2)
                continue;

            Strand strand;
            strand.positions.reserve(numPoints);
            strand.radius.reserve(numPoints);
            strand.arcLength = 0.0f;

            UT_Vector3 prevPos(0, 0, 0);
            bool firstPoint = true;

            for (int p = 0; p < numPoints; ++p)
            {
                float x, y, z;
                if (!(file >> x >> y >> z))
                    break;

                UT_Vector3 pos(x, y, z);
                strand.positions.push_back(pos);
                strand.radius.push_back(1.0f);

                if (!firstPoint)
                {
                    float segLen = (pos - prevPos).length();
                    strand.arcLength += segLen;
                }

                prevPos = pos;
                firstPoint = false;
            }

            if (strand.positions.size() >= 2)
            {
                strands.addStrand(strand);
            }
        }

        file.close();
        lastErrorMessage = "";
    }
    catch (const std::exception& e)
    {
        lastErrorMessage = std::string("Exception loading curve file: ") + e.what();
        return StrandSet();
    }

    return strands;
}

bool GeometryImporter::validateGeometry(const GU_Detail* gdp)
{
    if (!gdp)
        return false;

    if (gdp->getNumPrimitives() == 0)
    {
        lastErrorMessage = "Input has no primitives";
        return false;
    }

    if (gdp->getNumPoints() == 0)
    {
        lastErrorMessage = "Input has no points";
        return false;
    }

    return true;
}

std::string GeometryImporter::getLastError()
{
    return lastErrorMessage;
}

void GeometryImporter::extractCurvesFromDetail(const GU_Detail* gdp,
    StrandSet& outStrands)
{
    // Placeholder - will be implemented later with proper Houdini headers
}

Strand GeometryImporter::convertCurveToStrand(const GU_Detail* gdp,
    GA_Offset primoff)
{
    Strand strand;
    strand.clusterID = -1;
    strand.reconstructionError = 0.0f;
    return strand;
}