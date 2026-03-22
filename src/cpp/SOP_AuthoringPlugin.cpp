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

PRM_Template
SOP_AuthoringPlugin::myTemplateList[] = {

    // strand count, bounding box, status
    PRM_Template(PRM_STRING, 1, &strandsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &boundsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &statusName, PRMzeroDefaults),
    
    // change number of guide strands
    PRM_Template(PRM_INT, 1, new PRM_Name("num_guides", "Number of Guides"),
                 new PRM_Default(20)),
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

        // ====================================================================
        // TASK 2.1 - COMPUTE FEATURES (READY FOR K-MEANS)
        // ====================================================================

        // Compute feature vectors for all strands
        // This prepares the data for K-means clustering (which Ray will implement)
        //computeFeatures();

        // ====================================================================
        // END OF COOKING
        // ====================================================================

        // make sure to free input
        unlockInput(0);
    }

    boss->opEnd();
    return error();
}

// ============================================================================
// HELPER FUNCTIONS - IMPLEMENTATIONS
// ============================================================================

void SOP_AuthoringPlugin::loadGeometry(const GU_Detail* input_geo)
{
    // This is now integrated into cookMySop() above
    // Keeping stub for API compatibility
}

void SOP_AuthoringPlugin::computeFeatures()
{
    // TASK 2.1 - COMPUTE FEATURE VECTORS
    // This prepares features for clustering

    if (inputStrands.getStrandCount() == 0)
    {
        statusMessage = "ERROR: No strands loaded for feature computation";
        return;
    }

    // Use the FeatureComputation class to compute all features
    std::vector<Feature> features =
        FeatureComputation::computeAllFeatures(inputStrands);

    // Normalize features to zero mean, unit variance
    FeatureComputation::normalizeFeatures(features);

    // Features are now ready to be passed to K-means clustering
    // Once Ray implements K-means, we'll call:
    // clusterGuides(getNumGuides(now));
}

void SOP_AuthoringPlugin::clusterGuides(int numGuides)
{
    // TASK 2.2 - K-MEANS CLUSTERING
    // This will be implemented by Ray
    // Placeholder for now

    if (inputStrands.getStrandCount() == 0)
    {
        statusMessage = "ERROR: No strands to cluster";
        return;
    }

    // TODO: Ray will implement K-means clustering here
    // Once implemented, this will:
    // 1. Take feature vectors computed in computeFeatures()
    // 2. Run K-means with numGuides clusters
    // 3. Assign each strand to a cluster
    // 4. Call extractFromStrands() to create guides
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