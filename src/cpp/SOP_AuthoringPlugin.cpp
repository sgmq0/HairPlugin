#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>

#include <iostream>

#include "SOP_AuthoringPlugin.h"

void
newSopOperator(OP_OperatorTable* table)
{
    table->addOperator(
        new OP_Operator("authoringplugin",
            "Authoring Plugin",
            SOP_AuthoringPlugin::myConstructor,
            SOP_AuthoringPlugin::myTemplateList,
            1,
            1,
            nullptr,
            OP_FLAG_GENERATOR)
    );
}

static PRM_Name	strandsName("strand_count", "Number of Strands");
static PRM_Name	boundsName("bounding_box", "Bounds");
static PRM_Name	statusName("load_status", "Status");
static PRM_Name	extractGuides("extract_guides", "Extract Guides");

PRM_Template
SOP_AuthoringPlugin::myTemplateList[] = {

    // strand count, bounding box, status
    PRM_Template(PRM_STRING, 1, &strandsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &boundsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &statusName, PRMzeroDefaults),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep1", "Sep1")),

    // change number of guide strands
    PRM_Template(PRM_INT, 1, new PRM_Name("num_guides", "Number of Guides"),
                 new PRM_Default(20)),
    PRM_Template(PRM_CALLBACK, 1, &extractGuides, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onExtractGuidesCallback),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep2", "Sep2")),

    PRM_Template()
};

OP_Node*
SOP_AuthoringPlugin::myConstructor(OP_Network* net, const char* name, OP_Operator* op)
{
    return new SOP_AuthoringPlugin(net, name, op);
}

SOP_AuthoringPlugin::SOP_AuthoringPlugin(OP_Network* net,
    const char* name,
    OP_Operator* op)
    : SOP_Node(net, name, op)
{
    statusMessage = "Ready";
}

SOP_AuthoringPlugin::~SOP_AuthoringPlugin()
{
}

unsigned
SOP_AuthoringPlugin::disableParms()
{
    return 0;
}

void SOP_AuthoringPlugin::inputConnectChanged(int which_input) {
    //parent
    SOP_Node::inputConnectChanged(which_input);

    if (which_input == 0 && getInput(0)) {
        addExtraInput(getInput(0), OP_INTEREST_DATA);
        forceRecook();
    }
}

void SOP_AuthoringPlugin::setDisplayStrings(fpreal now, std::string strand_str, std::string bounds_str, std::string status_str) {
    setString(strand_str, CH_STRING_LITERAL, "strand_count", 0, now);
    setString(bounds_str, CH_STRING_LITERAL, "bounding_box", 0, now);
    setString(status_str, CH_STRING_LITERAL, "load_status", 0, now);
}

OP_ERROR
SOP_AuthoringPlugin::cookMySop(OP_Context& context)
{
    fpreal now = context.getTime();

    UT_Interrupt* boss = UTgetInterrupt();

    if (boss->opStart("Cooking AuthoringPlugin"))
    {
        gdp->clearAndDestroy();

        // ====================================================================
        // TASK 1.1 - LOAD GEOMETRY FROM UPSTREAM SOP
        // ====================================================================

        // lock the input so it's safe to access
        if (lockInput(0, context) >= UT_ERROR_ABORT)
        {
            boss->opEnd();
            return error();
        }

        // Get the input geometry from upstream SOP
        const GU_Detail* input_geo = inputGeo(0, context);

        if (!input_geo)
        {
            unlockInput(0);
            statusMessage = "Waiting for input geometry";
            setDisplayStrings(now, 0, statusMessage, "-");
            // Don't error - just waiting for connection
            boss->opEnd();
            return error();
        }

        // Check if input has any geometry
        if (input_geo->getNumPrimitives() == 0)
        {
            unlockInput(0);
            statusMessage = "FAILED: Input has no geometry";
            setDisplayStrings(now, 0, statusMessage, "-");
            boss->opEnd();
            return error();
        }

        // Load curves from Houdini geometry
        inputStrands = GeometryImporter::loadFromHoudiniGeometry(input_geo);

        if (!GeometryImporter::getLastError().empty())
        {
            addMessage(SOP_MESSAGE, GeometryImporter::getLastError().c_str());
        }

        // Validate that we loaded something
        if (inputStrands.getStrandCount() == 0)
        {
            unlockInput(0);
            addMessage(SOP_MESSAGE, "u messed up 3");
            statusMessage = "FAILED: No curves found in input";
            setDisplayStrings(now, 0, statusMessage, "-");
            boss->opEnd();
            return error();
        } 
        else {
            addMessage(SOP_MESSAGE, std::to_string(inputStrands.getStrandCount()).c_str());
        }

        // ====================================================================
        // TASK 1.3 - DISPLAY STATUS (Strand count + Bounding box)
        // ====================================================================

        // Get strand information
        int strandCount = inputStrands.getStrandCount();
        UT_BoundingBox bounds = inputStrands.getBounds();

        // Calculate size from bounding box
        fpreal width = bounds.xmax() - bounds.xmin();
        fpreal height = bounds.ymax() - bounds.ymin();
        fpreal depth = bounds.zmax() - bounds.zmin();

        // Create formatted status message
        char statusBuf[512];
        snprintf(statusBuf, sizeof(statusBuf),
            "Strands: %d | Bounds: %.2f x %.2f x %.2f",
            strandCount,
            width, height, depth);

        // Store status message for later retrieval
        statusMessage = statusBuf;

        // Display status message to user in Houdini
        addMessage(SOP_MESSAGE, statusBuf);

        // set all the ui information
        setDisplayStrings(now, std::to_string(inputStrands.getStrandCount()).c_str(), statusBuf, "OK");

        // make sure to free input
        unlockInput(0);
    }

    boss->opEnd();
    return error();
}

// ============================================================================
// HELPER FUNCTIONS - IMPLEMENTATIONS
// ============================================================================

int SOP_AuthoringPlugin::onExtractGuidesCallback(void* data, int index, fpreal t, const PRM_Template*) {
    // just a container for onExtractGuides, since this is static

    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onExtractGuides(t);

    return 1;
}

void SOP_AuthoringPlugin::onExtractGuides(fpreal t) {
    // TASK 2 - K MEANS GUIDE CLUSTERING
    // does all the other task 2 functions, connected to a button in the GUI

    std::vector<Feature> features = computeFeatures(); // task 2.1: feature vector computation
    clusterGuides(getNumGuides(t), features); // task 2.2: run k means clustering
    smoothGuides(); 

}

std::vector<Feature> SOP_AuthoringPlugin::computeFeatures()
{
    // TASK 2.1 - COMPUTE FEATURE VECTORS
    // This prepares features for clustering
    if (inputStrands.getStrandCount() == 0)
    {
        statusMessage = "ERROR: No strands loaded for feature computation";
        return std::vector<Feature>();
    }

    // Use the FeatureComputation class to compute all features
    std::vector<Feature> features =
        FeatureComputation::computeAllFeatures(inputStrands);

    // Normalize features to zero mean, unit variance
    FeatureComputation::normalizeFeatures(features);

    // Features are now ready to be passed to K-means clustering
    addMessage(SOP_MESSAGE, "Features computed");

    return features;
}

void SOP_AuthoringPlugin::clusterGuides(int numGuides, std::vector<Feature> features)
{
    // TASK 2.2 - K-MEANS CLUSTERING
    // implemented by ray
    // doing k means clustering manually
    // 1. Take feature vectors computed in computeFeatures()
    // 2. Run K-means with numGuides clusters
    // 3. Assign each strand to a cluster
    // 4. Call extractFromStrands() to create guides

    addMessage(SOP_MESSAGE, "Begin K-means clustering");

    if (inputStrands.getStrandCount() == 0)
    {
        statusMessage = "ERROR: No strands to cluster";
        return;
    }

    int numStrands = inputStrands.getStrandCount();
    int k = std::min(numGuides, numStrands); // min because k can't be less than numStrands
    int featureDim = features[0].values.size(); // 7

    // 1. initialize cluster centers randomly
    std::vector<int> centerIndices;
    std::srand(67); // lol

    // add first center
    centerIndices.push_back(std::rand() % numStrands);

    for (int c = 1; c < k; c++) {

        // Compute distance from each strand to nearest existing center
        std::vector<float> minDists(numStrands, FLT_MAX);
        for (int i = 0; i < numStrands; ++i) {
            for (int ci : centerIndices)    {
                float dist = features[i].featureDistance(features[ci]);
                minDists[i] = std::min(minDists[i], dist);
            }
        }

        // Pick next center weighted by distance squared
        std::vector<float> weights(numStrands);
        float totalWeight = 0.0f;
        for (int i = 0; i < numStrands; ++i)
        {
            weights[i] = minDists[i] * minDists[i];
            totalWeight += weights[i];
        }

        float r = ((float)std::rand() / RAND_MAX) * totalWeight;
        float cumulative = 0.0f;
        for (int i = 0; i < numStrands; ++i)
        {
            cumulative += weights[i];
            if (cumulative >= r)
            {
                centerIndices.push_back(i);
                break;
            }
        }
    }

    // Initialize cluster centers from chosen strands
    std::vector<std::array<float, 7>> centers(k);
    for (int c = 0; c < k; ++c)
        centers[c] = features[centerIndices[c]].values;

    // 2: iteration step
    std::vector<int> assignments(numStrands, -1);
    const int maxIterations = 100;

    for (int iter = 0; iter < maxIterations; ++iter)
    {
        bool changed = false;

        // Assign each strand to nearest center
        for (int i = 0; i < numStrands; ++i)
        {
            float bestDist = FLT_MAX;
            int bestCluster = 0;

            for (int c = 0; c < k; ++c)
            {
                float dist = 0.0f;
                for (int d = 0; d < featureDim; ++d)
                {
                    float diff = features[i].values[d] - centers[c][d];
                    dist += diff * diff;
                }

                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestCluster = c;
                }
            }

            if (assignments[i] != bestCluster)
            {
                assignments[i] = bestCluster;
                changed = true;
            }
        }

        // Stop early if nothing changed
        if (!changed)
        {
            addMessage(SOP_MESSAGE, ("K-means converged at iteration " +
                std::to_string(iter)).c_str());
            break;
        }

        // Recompute centers as mean of assigned strands
        std::vector<std::array<float, 7>> newCenters(k);
        for (int c = 0; c < k; ++c)
            newCenters[c].fill(0.0f);

        std::vector<int> clusterSizes(k, 0);

        for (int i = 0; i < numStrands; ++i)
        {
            int c = assignments[i];
            for (int d = 0; d < featureDim; ++d)
                newCenters[c][d] += features[i].values[d];
            clusterSizes[c]++;
        }

        for (int c = 0; c < k; ++c)
        {
            if (clusterSizes[c] > 0)
            {
                for (int d = 0; d < featureDim; ++d)
                    newCenters[c][d] /= clusterSizes[c];
            }
            else
            {
                // Empty cluster - reinitialize to a random strand
                newCenters[c] = features[std::rand() % numStrands].values;
            }
        }

        centers = newCenters;
    }

    // 3: add everything to a vector of guide strands
    std::vector<Strand> guideStrands;

    for (int c = 0; c < k; ++c)
    {
        float bestDist = FLT_MAX;
        int bestStrand = 0;

        for (int i = 0; i < numStrands; ++i)
        {
            if (assignments[i] != c) continue;

            float dist = 0.0f;
            for (int d = 0; d < featureDim; ++d)
            {
                float diff = features[i].values[d] - centers[c][d];
                dist += diff * diff;
            }

            if (dist < bestDist)
            {
                bestDist = dist;
                bestStrand = i;
            }
        }

        Strand& s = inputStrands.getStrand(bestStrand);
        s.clusterID = c;
        guideStrands.push_back(s);
    }

    guides.extractFromStrands(guideStrands, k);

    addMessage(SOP_MESSAGE, "K-means clustering complete");
    addMessage(SOP_MESSAGE, std::to_string(guides.getGuideCount()).c_str());
}

void SOP_AuthoringPlugin::smoothGuides()
{
    // TASK 2.3 - GUIDE SMOOTHING
    // Extract guides from clusters and smooth them

    if (guides.getGuideCount() == 0)
    {
        statusMessage = "ERROR: No guides to smooth";
        return;
    }

    // Guides are created in clusterGuides(), this smooths them
    // B-spline smoothing is handled by GuideSmoothing class
}

void SOP_AuthoringPlugin::synthesizeHair()
{
    // TASK 3 - HAIR SYNTHESIS (BETA FEATURE)
    // Not implemented in Alpha
}

void SOP_AuthoringPlugin::displayStrandSet(GU_Detail* gdp, const StrandSet& strands)
{
    // TASK 1.3 - DISPLAY INPUT GEOMETRY (VISUALIZATION)
    // This will render input curves as white wireframe
    // To be implemented later
}

void SOP_AuthoringPlugin::displayGuides(GU_Detail* gdp, const GuideSet& guides)
{
    // TASK 2.4 - DISPLAY GUIDE CURVES (VISUALIZATION)
    // This will render guide curves as red overlay
    // To be implemented by Ray
}

void SOP_AuthoringPlugin::displaySynthesized(GU_Detail* gdp, const StrandSet& synthesized)
{
    // TASK 3 - DISPLAY SYNTHESIZED HAIR (BETA FEATURE)
    // Not implemented in Alpha
}