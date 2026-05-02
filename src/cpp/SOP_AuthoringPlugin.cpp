#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <GEO/GEO_PrimPoly.h>
#include <GU/GU_RayIntersect.h>

#include <iostream>

#include "SOP_AuthoringPlugin.h"
#include "MeshToStrands.h"
#include "ClusterVisualization.h"
#include "ClumpOperator.h"
#include "LossComputation.h"
#include "SimpleOptimizer.h"
#include "ExportManager.h"

void
newSopOperator(OP_OperatorTable* table)
{
    table->addOperator(
        new OP_Operator("authoringplugin",
            "Authoring Plugin",
            SOP_AuthoringPlugin::myConstructor,
            SOP_AuthoringPlugin::myTemplateList,
            2,          // minimum 2 inputs
            2,          // maximum 2 inputs
            nullptr)
    );
}

static PRM_Name	strandsName("strand_count", "Number of Strands");
static PRM_Name	boundsName("bounding_box", "Bounds");
static PRM_Name	statusName("load_status", "Status");

// Week 5 - Mesh Loading
static PRM_Name hairMeshFileName("hair_mesh_file", "Hair Mesh (OBJ)");
static PRM_Name loadHairMeshBtn("load_hair_mesh", "Load Hair Mesh");

// Week 5 - Cluster Visualization
static PRM_Name colorByClusterName("color_by_cluster", "Color by Cluster");
static PRM_Name selectedGuideName("selected_guide", "Selected Guide");
static PRM_Range selectedGuideRange(PRM_RANGE_RESTRICTED, -1, PRM_RANGE_UI, 100);

static PRM_Name numGuidesName("num_guides", "Number of Guides");
static PRM_Range numGuidesRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_UI, 100);
static PRM_Name	extractGuidesBtn("extract_guides", "Extract Guides");

static PRM_Name radiusName("clump_radius", "Clump Radius");
static PRM_Range radiusRange(PRM_RANGE_RESTRICTED, 0.1f, PRM_RANGE_UI, 10.0f);
static PRM_Name tightnessName("clump_tightness", "Clump Tightness");
static PRM_Range tightnessRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 1.0f);
static PRM_Name countName("clump_count", "Clump Count");
static PRM_Range countRange(PRM_RANGE_RESTRICTED, 1, PRM_RANGE_UI, 100);
static PRM_Name synthesizeHairBtn("synthesize_strands", "Synthesize Hair");

// Twist operator
static PRM_Name twistAmountName("twist_amount", "Twist Amount");
static PRM_Range twistAmountRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 360.0f);
static PRM_Name twistFrequencyName("twist_frequency", "Twist Frequency");
static PRM_Range twistFrequencyRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 10.0f);

// Bend operator
static PRM_Name bendMagnitudeName("bend_magnitude", "Bend Magnitude");
static PRM_Range bendMagnitudeRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 1.0f);
static PRM_Name bendDirXName("bend_dir_x", "Bend X");
static PRM_Range bendDirXRange(PRM_RANGE_RESTRICTED, -1.0f, PRM_RANGE_UI, 1.0f);
static PRM_Name bendDirYName("bend_dir_y", "Bend Y");
static PRM_Range bendDirYRange(PRM_RANGE_RESTRICTED, -1.0f, PRM_RANGE_UI, 1.0f);
static PRM_Name bendDirZName("bend_dir_z", "Bend Z");
static PRM_Range bendDirZRange(PRM_RANGE_RESTRICTED, -1.0f, PRM_RANGE_UI, 1.0f);

// Noise operator
static PRM_Name noiseAmplitudeName("noise_amplitude", "Noise Amplitude");
static PRM_Range noiseAmplitudeRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 1.0f);
static PRM_Name noiseFrequencyName("noise_frequency", "Noise Frequency");
static PRM_Range noiseFrequencyRange(PRM_RANGE_RESTRICTED, 0.1f, PRM_RANGE_UI, 10.0f);

// Optimization
static PRM_Name optEnableName("opt_enable", "Enable Optimization");
static PRM_Name optIterationsName("opt_iterations", "Optimization Iterations");
static PRM_Range optIterationsRange(PRM_RANGE_RESTRICTED, 1, PRM_RANGE_UI, 100);
static PRM_Name optimizeBtn("optimize", "Run Optimization");

// Export
static PRM_Name exportPathName("export_path", "Export Path");
static PRM_Name exportMetricsName("export_metrics", "Metrics");
static PRM_Name exportBtn("export", "Export Synthesized Hair");

PRM_Template
SOP_AuthoringPlugin::myTemplateList[] = {

    // strand count, bounding box, status
    PRM_Template(PRM_STRING, 1, &strandsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &boundsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &statusName, PRMzeroDefaults),

    // Hair Mesh Loading
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_input", "--- Input ---")),
    PRM_Template(PRM_FILE, 1, &hairMeshFileName, new PRM_Default(0, "*.obj")),
    PRM_Template(PRM_CALLBACK, 1, &loadHairMeshBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onLoadHairMeshCallback),


    // change number of guide strands
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_guides", "��� Guides ���")),
    PRM_Template(PRM_INT, 1, &numGuidesName, new PRM_Default(20), 0, &numGuidesRange),
    PRM_Template(PRM_CALLBACK, 1, &extractGuidesBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onExtractGuidesCallback),

    // clump parameters
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_synth", "��� Synthesis ���")),
    PRM_Template(PRM_FLT, 1, &radiusName, new PRM_Default(0.1), 0, &radiusRange),
    PRM_Template(PRM_FLT, 1, &tightnessName, new PRM_Default(1.0), 0, &tightnessRange),
    PRM_Template(PRM_INT, 1, &countName, new PRM_Default(80), 0, &countRange),

    // Twist parameters
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_twist", "--- Twist ---")),
    PRM_Template(PRM_FLT, 1, &twistAmountName, new PRM_Default(0.0), 0, &twistAmountRange),
    PRM_Template(PRM_FLT, 1, &twistFrequencyName, new PRM_Default(1.0), 0, &twistFrequencyRange),

    // Bend parameters
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_bend", "--- Bend ---")),
    PRM_Template(PRM_FLT, 1, &bendMagnitudeName, new PRM_Default(0.0), 0, &bendMagnitudeRange),
    PRM_Template(PRM_FLT, 1, &bendDirXName, new PRM_Default(0.0), 0, &bendDirXRange),
    PRM_Template(PRM_FLT, 1, &bendDirYName, new PRM_Default(0.0), 0, &bendDirYRange),
    PRM_Template(PRM_FLT, 1, &bendDirZName, new PRM_Default(1.0), 0, &bendDirZRange),

    // Noise parameters
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_noise", "--- Noise ---")),
    PRM_Template(PRM_FLT, 1, &noiseAmplitudeName, new PRM_Default(0.0), 0, &noiseAmplitudeRange),
    PRM_Template(PRM_FLT, 1, &noiseFrequencyName, new PRM_Default(1.0), 0, &noiseFrequencyRange),

    PRM_Template(PRM_CALLBACK, 1, &synthesizeHairBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onSynthesizeHairCallback),

    // Week 5 - Cluster Visualization
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_viz", "��� Visualization ���")),
    PRM_Template(PRM_TOGGLE, 1, &colorByClusterName, new PRM_Default(1)),
    PRM_Template(PRM_INT, 1, &selectedGuideName, new PRM_Default(-1), 0, &selectedGuideRange),

    // Optimization
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_opt", "--- Optimization ---")),
    PRM_Template(PRM_TOGGLE, 1, &optEnableName, new PRM_Default(0)),
    PRM_Template(PRM_INT, 1, &optIterationsName, new PRM_Default(20), 0, &optIterationsRange),
    PRM_Template(PRM_CALLBACK, 1, &optimizeBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onOptimizeCallback),

    // Export
    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_export", "--- Export ---")),
    PRM_Template(PRM_STRING, 1, &exportMetricsName, PRMzeroDefaults),
    PRM_Template(PRM_FILE, 1, &exportPathName, new PRM_Default(0, "$HOME/synthesized_hair.obj")),
    PRM_Template(PRM_CALLBACK, 1, &exportBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onExportCallback),
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

// Week 5 - Load Hair Mesh Callback
int SOP_AuthoringPlugin::onLoadHairMeshCallback(void* data, int index, fpreal t, const PRM_Template*) {
    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onLoadHairMesh(t);
    return 1;
}

void SOP_AuthoringPlugin::onLoadHairMesh(fpreal t) {
    // Get file path from parameter
    UT_String filepath;
    evalString(filepath, "hair_mesh_file", 0, t);

    if (filepath.length() == 0) {
        addMessage(SOP_MESSAGE, "Please select a hair mesh file");
        return;
    }

    // Load mesh
    inputStrands = MeshToStrands::loadHairMeshFromOBJ(filepath.toStdString());

    if (inputStrands.getStrandCount() == 0) {
        addMessage(SOP_MESSAGE, ("Failed to load mesh: " +
            std::string(MeshToStrands::getLastError())).c_str());
        return;
    }

    addMessage(SOP_MESSAGE, ("Loaded " +
        std::to_string(inputStrands.getStrandCount()) +
        " strands from mesh").c_str());

    // Reset guides and synthesis when loading new mesh
    guides = GuideSet();
    synthesizedStrands = StrandSet();
    guidesReady = false;
    synthesisReady = false;

    forceRecook();
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

    scalpConnected = (getInput(1) != nullptr);

    if (boss->opStart("Cooking AuthoringPlugin"))
    {
        gdp->clearAndDestroy();

        // Check if we have loaded strands from file
        // No input geometry required - work standalone
        if (lockInput(0, context) >= UT_ERROR_ABORT)
        {
            statusMessage = "Load a hair mesh file to begin";
            setDisplayStrings(now, "0", "-", statusMessage);
            boss->opEnd();
            return error();
        }

        if (!inputLoaded) {
            // =========== TASK 1.1 - LOAD GEOMETRY FROM UPSTREAM SOP ===========

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

            // =========== TASK 1.3 - DISPLAY STATUS (Strand count + Bounding box) ===========

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
            setDisplayStrings(now, std::to_string(inputStrands.getStrandCount()), statusBuf, "OK");

            inputLoaded = true;
        }

        // check if scalp mesh is connected (optional)
        if (lockInput(1, context) < UT_ERROR_ABORT)
        {
            scalpConnected = (getInput(1) != nullptr);

            // load in scalp mesh if connected
            if (scalpConnected) {
                // Get the input geometry from upstream SOP
                const GU_Detail* input_geo = inputGeo(1, context);

                if (input_geo && input_geo->getNumPrimitives() > 0)
                {
                    GA_Index numPrims = gdp->getNumPrimitives();

                    // show the scalp
                    gdp->copy(*input_geo, GEO_COPY_ADD);

                    // add scalp to its own group so we can isolate it later
                    GA_PrimitiveGroup* scalpGroup = gdp->newPrimitiveGroup("scalp");
                    for (GA_Iterator it(gdp->getPrimitiveRange()); !it.atEnd(); ++it)
                    {
                        const GA_Primitive* prim = gdp->getPrimitive(*it);
                        if (prim->getMapIndex() >= numPrims)
                            scalpGroup->add(prim);
                    }
                }
            }
            unlockInput(1);
        }

        // Display based on state
        if (synthesisReady) {
            // Check if any clump parameters have changed
            float currentRadius = getClumpRadius(now);
            float currentTightness = getClumpTightness(now);
            int currentCount = getClumpCount(now);
            float currentTwistAmount = getTwistAmount(now);
            float currentTwistFrequency = getTwistFrequency(now);
            float currentBendMagnitude = getBendMagnitude(now);
            float currentNoiseAmplitude = getNoiseAmplitude(now);
            float currentNoiseFrequency = getNoiseFrequency(now);

            // If any parameter changed, re-synthesize
            if (std::abs(currentRadius - cachedClumpRadius) > 1e-6f ||
                std::abs(currentTightness - cachedClumpTightness) > 1e-6f ||
                currentCount != cachedClumpCount ||
                std::abs(currentTwistAmount - cachedTwistAmount) > 1e-6f ||
                std::abs(currentTwistFrequency - cachedTwistFrequency) > 1e-6f ||
                std::abs(currentBendMagnitude - cachedBendMagnitude) > 1e-6f ||
                std::abs(getBendDirX(now) - cachedBendDirX) > 1e-6f ||
                std::abs(getBendDirY(now) - cachedBendDirY) > 1e-6f ||
                std::abs(getBendDirZ(now) - cachedBendDirZ) > 1e-6f ||
                std::abs(currentNoiseAmplitude - cachedNoiseAmplitude) > 1e-6f ||
                std::abs(currentNoiseFrequency - cachedNoiseFrequency) > 1e-6f) {

                // Parameters changed - re-synthesize with new values
                ClumpParams params;
                params.clumpRadius = currentRadius;
                params.clumpTightness = currentTightness;
                params.clumpCount = currentCount;
                params.twistAmount = currentTwistAmount;
                params.twistFrequency = currentTwistFrequency;
                params.bendMagnitude = currentBendMagnitude;
                params.bendDirection.assign(
                    getBendDirX(now),
                    getBendDirY(now),
                    getBendDirZ(now));
                params.noiseAmplitude = currentNoiseAmplitude;
                params.noiseFrequency = currentNoiseFrequency;

                // Re-synthesize
                synthesizedStrands = ClumpOperator::clumpFromGuides(guides, params);

                // Update cached values
                cachedClumpRadius = currentRadius;
                cachedClumpTightness = currentTightness;
                cachedClumpCount = currentCount;
                cachedTwistAmount = currentTwistAmount;
                cachedTwistFrequency = currentTwistFrequency;
                cachedBendMagnitude = currentBendMagnitude;
                cachedBendDirX = getBendDirX(now);
                cachedBendDirY = getBendDirY(now);
                cachedBendDirZ = getBendDirZ(now);
                cachedNoiseAmplitude = currentNoiseAmplitude;
                cachedNoiseFrequency = currentNoiseFrequency;
            }

            synthesizeHair(gdp);

            addMessage(SOP_MESSAGE, ("Synthesized " +
                std::to_string(synthesizedStrands.getStrandCount()) +
                " strands").c_str());

            // Show all three layers: input, guides, and synthesis
            displayStrandSet(gdp, synthesizedStrands);
            displayGuides(gdp, guides);
            addMessage(SOP_MESSAGE, ("Showing " + std::to_string(synthesizedStrands.getStrandCount()) + " synthesized strands").c_str());
        }

        if (guidesReady) {
            // compute the UV location (on the scalp) of each of the guide strands' roots 
            guides.computeUVLocations(gdp);

            UT_Vector2 uv = guides.getGuide(1).root_UV;
            std::string uvStr = "(" + std::to_string(uv.x()) + ", " + std::to_string(uv.y()) + ")";
            addMessage(SOP_MESSAGE, uvStr.c_str());

            // compute kd tree of guides
            closestGuides.fillKDTree(guides);

            // Show input strands and guides
            displayGuides(gdp, guides);
            addMessage(SOP_MESSAGE, "guides displayed");
        }

        if (!guidesReady && !synthesisReady) {
            // just display input by default
            displayStrandSet(gdp, inputStrands);
        }

        // make sure to free input
        unlockInput(0);
        unlockInput(1);
    }

    boss->opEnd();
    return error();
}

// ============================================================================
//                             CALLBACK FUNCTIONS
// ============================================================================

int SOP_AuthoringPlugin::onExtractGuidesCallback(void* data, int index, fpreal t, const PRM_Template*) {
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

    guidesReady = true;
    synthesisReady = false;  // Reset synthesis when guides change
    forceRecook();
}

// Callback for Synthesize Hair button
int SOP_AuthoringPlugin::onSynthesizeHairCallback(void* data, int index, fpreal t, const PRM_Template*) {
    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onSynthesizeHair(t);
    return 1;
}

void SOP_AuthoringPlugin::onSynthesizeHair(fpreal t) {
    // TASK 3 - Generate clumped hair from guides

    if (guides.getGuideCount() == 0) {
        addMessage(SOP_MESSAGE, "No guides to synthesize from. Extract guides first.");
        return;
    }

    extractRootsFromInputStrands();

    // Build full clumping parameters
    ClumpParams params;
    params.clumpRadius = getClumpRadius(t);
    params.clumpTightness = getClumpTightness(t);
    params.clumpCount = getClumpCount(t);

    // Twist operator
    params.twistAmount = getTwistAmount(t);
    params.twistFrequency = getTwistFrequency(t);

    // Bend operator
    params.bendMagnitude = getBendMagnitude(t);
    params.bendDirection.assign(
        getBendDirX(t),
        getBendDirY(t),
        getBendDirZ(t));

    // Noise operator
    params.noiseAmplitude = getNoiseAmplitude(t);
    params.noiseFrequency = getNoiseFrequency(t);

    // Generate clumped strands from guides
    synthesizedStrands = ClumpOperator::clumpFromGuides(guides, params);

    if (synthesizedStrands.getStrandCount() == 0) {
        addMessage(SOP_MESSAGE, "Failed to synthesize hair");
        return;
    }

    synthesisReady = true;

    // Cache current parameters for change detection
    cachedClumpRadius = getClumpRadius(t);
    cachedClumpTightness = getClumpTightness(t);
    cachedClumpCount = getClumpCount(t);
    cachedTwistAmount = getTwistAmount(t);
    cachedTwistFrequency = getTwistFrequency(t);
    cachedBendMagnitude = getBendMagnitude(t);
    cachedBendDirX = getBendDirX(t);
    cachedBendDirY = getBendDirY(t);
    cachedBendDirZ = getBendDirZ(t);
    cachedNoiseAmplitude = getNoiseAmplitude(t);
    cachedNoiseFrequency = getNoiseFrequency(t);

    forceRecook();
}

// ============================================================================
//                            CLUSTERING FUNCTIONS
// ============================================================================

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
    std::srand(67);

    // add first center
    centerIndices.push_back(std::rand() % numStrands);

    for (int c = 1; c < k; c++) {

        // Compute distance from each strand to nearest existing center
        std::vector<float> minDists(numStrands, FLT_MAX);
        for (int i = 0; i < numStrands; ++i) {
            for (int ci : centerIndices) {
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

            // assign cluster ids to strands
            Strand& strand = inputStrands.getStrand(i);
            strand.clusterID = assignments[i];
        }

        Strand& s = inputStrands.getStrand(bestStrand);
        s.clusterID = c;
        guideStrands.push_back(s);
    }

    guides.extractFromStrands(guideStrands, k);

    // Week 5 - Assign cluster IDs to all strands
    // ClusterVisualization::assignClustersToStrands(inputStrands, guides);

    addMessage(SOP_MESSAGE, "K-means clustering complete");
    addMessage(SOP_MESSAGE, std::to_string(guides.getGuideCount()).c_str());
}

// ============================================================================
//                          HELPER/UTILITY FUNCTIONS
// ============================================================================

void SOP_AuthoringPlugin::smoothGuides()
{
    // TASK 2.3 - GUIDE SMOOTHING
    if (guides.getGuideCount() == 0)
    {
        statusMessage = "ERROR: No guides to smooth";
        return;
    }
}

void SOP_AuthoringPlugin::synthesizeHair(GU_Detail* gdp)
{
    // TASK 3 - HAIR SYNTHESIS
    synthesizedStrands.clear();

    const GA_PrimitiveGroup* scalpGroup = gdp->findPrimitiveGroup("scalp");
    GU_RayIntersect ray(gdp, scalpGroup);

    // default line
    int numVertices = guides.getGuide(0).positions.size();

    // populate synthesizedStrands
    for (size_t i = 0; i < strandRoots.size(); i++) {
        Strand strand;
        UT_Vector3 root = strandRoots.at(i);

        // find strand's uv position
        GU_MinInfo minInfo;
        ray.minimumPoint(root, minInfo);
        strand.computeUVLocation(gdp, &minInfo);

        // assign strand N guide strands that influence it
        int N = 4;
        strand.guides = closestGuides.findNNearest(strand.root_UV, N);

        // compute the guide strands' weights
        strand.computeGuideWeights(&guides);

        // find strand's position vector 
        strand.positions.push_back(root);   // x_(i,1)

        UT_Vector3 numSum0 = { 0.0f, 0.0f, 0.0f };
        float denomSum0 = 0.0f;
        for (std::pair<float, int> g : strand.guideWeights) {
            // weight of guide
            float weight = g.first;
            denomSum0 += weight;

            // position of guide's j-th vertex
            float idx = g.second;
            UT_Vector3 pos = guides.getGuide(idx).positions.at(0);
            numSum0 += (pos * weight);
        }
        UT_Vector3 g_i0 = numSum0 / denomSum0;

        for (size_t j = 1; j < numVertices; j++) {
            UT_Vector3 finalPos = root;

            // numerator: sum up weight of guide * position of guide's j-th vertex
            // denominator: sum up weights of guides
            UT_Vector3 numSum = { 0.0f, 0.0f, 0.0f };
            float denomSum = 0.0f;
            for (std::pair<float, int> g : strand.guideWeights) {
                // weight of guide
                float weight = g.first;
                denomSum += weight;

                // position of guide's j-th vertex
                float idx = g.second;
                UT_Vector3 pos = guides.getGuide(idx).positions.at(j);
                numSum += (pos * weight);
            }
            UT_Vector3 g_ij = numSum / denomSum;
            finalPos += g_ij;
            finalPos -= g_i0;

            strand.positions.push_back(finalPos);

            // push back arbitrary radius
            strand.radius.push_back(1.0f);
        }

        // figure out arclength
        strand.arcLength = 0.0f;
        for (size_t j = 1; j < strand.positions.size(); j++)
        {
            float segLen = (strand.positions[j] - strand.positions[j - 1]).length();
            strand.arcLength += segLen;
        }

        // find which cluster it belongs to
        strand.clusterID = inputStrands.getStrand(i).clusterID;

        if (strand.arcLength > 1e-6f)
        {
            synthesizedStrands.addStrand(strand);
        }
    }
}

void SOP_AuthoringPlugin::extractRootsFromInputStrands()
{
    strandRoots.clear();
    for (int i = 0; i < inputStrands.getStrandCount(); i++) {
        strandRoots.push_back(inputStrands.getStrand(i).getRoot());
    }
}

// ============================================================================
//                             DISPLAY FUNCTIONS
// ============================================================================

void SOP_AuthoringPlugin::displayStrandSet(GU_Detail* gdp, const StrandSet& strands)
{
    // TASK 1.3 - DISPLAY INPUT GEOMETRY (VISUALIZATION)
    // Week 5 - Color by cluster

    if (!gdp) return;

    // Get visualization settings
    bool colorByCluster = getColorByCluster(0);
    int selectedCluster = getSelectedGuideIndex(0);

    // Generate cluster colors if needed
    std::vector<UT_Vector3> clusterColors;
    if (colorByCluster && guides.getGuideCount() > 0) {
        clusterColors = ClusterVisualization::generateClusterColors(guides.getGuideCount());
    }

    GA_RWHandleV3 colorHandle(gdp->addDiffuseAttribute(GA_ATTRIB_PRIMITIVE));

    // Render each strand
    for (int i = 0; i < strands.getStrandCount(); ++i) {
        const Strand& strand = strands.getStrand(i);
        if (strand.positions.size() < 2) continue;

        // Get color for this strand
        UT_Vector3 strandColor;
        float brightness = 1.0f;

        if (colorByCluster && guides.getGuideCount() > 0) {
            // Color by cluster
            strandColor = ClusterVisualization::getClusterColor(
                strand.clusterID,
                guides.getGuideCount());

            // Apply highlight factor if guide selected
            brightness = ClusterVisualization::getHighlightFactor(
                strand.clusterID,
                selectedCluster,
                selectedCluster >= 0);

            strandColor *= brightness;
        }
        else {
            // Default white color
            //strandColor = UT_Vector3(1.0f, 1.0f, 1.0f);

            UT_Vector2 uv = strand.root_UV;
            strandColor.assign(uv.x(), uv.y(), 1.0f);
        }

        // Create geometry
        GA_Offset startPt = gdp->appendPointBlock(strand.positions.size());
        for (int p = 0; p < (int)strand.positions.size(); ++p)
            gdp->setPos3(startPt + p, strand.positions[p]);

        GEO_PrimPoly* prim = GEO_PrimPoly::build(gdp, strand.positions.size(), GU_POLY_OPEN, false);
        for (int p = 0; p < (int)strand.positions.size(); ++p)
            prim->setPointOffset(p, startPt + p);

        // Apply color
        if (colorHandle.isValid())
            colorHandle.set(prim->getMapOffset(), strandColor);
    }
}

void SOP_AuthoringPlugin::displayGuides(GU_Detail* gdp, const GuideSet& guides)
{
    // TASK 2.4 - DISPLAY GUIDE CURVES (RED)

    if (!gdp) return;

    GA_RWHandleV3 colorHandle(gdp->addDiffuseAttribute(GA_ATTRIB_PRIMITIVE));

    // iterate over guide curves
    for (int i = 0; i < guides.getGuideCount(); i++) {
        const Strand& guide = guides.getGuide(i);
        if (guide.positions.size() < 2) continue;

        GA_Offset startPt = gdp->appendPointBlock(guide.positions.size());
        for (int p = 0; p < (int)guide.positions.size(); p++)
            gdp->setPos3(startPt + p, guide.positions[p]);

        GEO_PrimPoly* prim = GEO_PrimPoly::build(gdp, guide.positions.size(), GU_POLY_OPEN, false);
        for (int p = 0; p < (int)guide.positions.size(); p++)
            prim->setPointOffset(p, startPt + p);

        // red
        if (colorHandle.isValid()) {
            UT_Vector3 redColor;
            redColor.assign(1.0f, 0.0f, 0.0f);
            colorHandle.set(prim->getMapOffset(), redColor);
        }
    }
}

// ============================================================================
// TASK 4: OPTIMIZATION FUNCTIONS
// ============================================================================

int SOP_AuthoringPlugin::onOptimizeCallback(void* data, int index, fpreal t, const PRM_Template*) {
    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onOptimize(t);
    return 1;
}

void SOP_AuthoringPlugin::onOptimize(fpreal t) {
    if (!synthesisReady) {
        addMessage(SOP_MESSAGE, "Synthesis not ready. Synthesize hair first.");
        return;
    }

    int maxIterations = evalInt("opt_iterations", 0, t);
    if (maxIterations <= 0) maxIterations = 20;

    addMessage(SOP_MESSAGE, "Starting optimization...");

    // Initialize optimizer with current parameters
    float clumpRadius = getClumpRadius(t);
    float clumpTightness = getClumpTightness(t);
    int clumpCount = getClumpCount(t);

    // Run optimization loop
    SimpleOptimizer optimizer;
    for (int iter = 0; iter < maxIterations; ++iter) {
        // Step tweaks the parameters in the optimizer
        optimizer.step(
            guides,
            clumpRadius,
            clumpTightness,
            clumpCount,
            inputStrands,
            synthesizedStrands);

        LossComponents loss = optimizer.getCurrentLoss();

        std::string msg = "Iteration " + std::to_string(iter + 1) +
            ": Loss = " + std::to_string(loss.totalLoss);
        addMessage(SOP_MESSAGE, msg.c_str());

        if (!optimizer.isImproving()) {
            addMessage(SOP_MESSAGE, "No improvement, stopping optimization.");
            break;
        }
    }

    // Get final optimized parameters from optimizer
    synthesizedStrands = optimizer.getSynthesizedStrands();

    addMessage(SOP_MESSAGE, "Optimization complete!");
    forceRecook();
}

// ============================================================================
// TASK 5: EXPORT FUNCTIONS
// ============================================================================

int SOP_AuthoringPlugin::onExportCallback(void* data, int index, fpreal t, const PRM_Template*) {
    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onExport(t);
    return 1;
}

void SOP_AuthoringPlugin::onExport(fpreal t) {
    if (!synthesisReady) {
        addMessage(SOP_MESSAGE, "Synthesis not ready. Synthesize hair first.");
        return;
    }

    // Get export path from UI
    UT_String exportPath;
    evalString(exportPath, "export_path", 0, t);

    if (exportPath.length() == 0) {
        addMessage(SOP_MESSAGE, "No export path specified.");
        return;
    }

    // Expand variables like $HOME
    std::string filepath = exportPath.toStdString();
    if (filepath.find("$HOME") != std::string::npos) {
        const char* home = std::getenv("HOME");
        if (!home) home = std::getenv("USERPROFILE");  // Windows
        if (home) {
            size_t pos = filepath.find("$HOME");
            filepath.replace(pos, 5, home);
        }
    }

    // Export synthesized strands to OBJ
    bool success = ExportManager::exportSynthesizedHair(synthesizedStrands, filepath);

    if (success) {
        std::string msg = "Export successful: " + filepath;
        addMessage(SOP_MESSAGE, msg.c_str());

        // Show metrics in message
        std::string metrics = "Input: " + std::to_string(inputStrands.getStrandCount()) +
            " strands → Output: " + std::to_string(synthesizedStrands.getStrandCount()) +
            " strands";
        addMessage(SOP_MESSAGE, metrics.c_str());
    }
    else {
        addMessage(SOP_MESSAGE, "Export failed!");
    }
}