#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <GEO/GEO_PrimPoly.h>

#include <iostream>

#include "SOP_AuthoringPlugin.h"
#include "MeshToStrands.h"
#include "ClusterVisualization.h"
#include "ClumpOperator.h"

void
newSopOperator(OP_OperatorTable* table)
{
    table->addOperator(
        new OP_Operator("authoringplugin",
            "Authoring Plugin",
            SOP_AuthoringPlugin::myConstructor,
            SOP_AuthoringPlugin::myTemplateList,
            0,
            0,
            nullptr,
            OP_FLAG_GENERATOR)
    );
}

// ============================================================================
// PARAMETER DECLARATIONS
// ============================================================================

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

// CLUMP OPERATION PARAMETERS
static PRM_Name clumpRadiusName("clump_radius", "Clump Radius");
static PRM_Range clumpRadiusRange(PRM_RANGE_RESTRICTED, 0.1f, PRM_RANGE_UI, 10.0f);

static PRM_Name clumpTightnessName("clump_tightness", "Clump Tightness");
static PRM_Range clumpTightnessRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 1.0f);

static PRM_Name clumpCountName("clump_count", "Clump Count");
static PRM_Range clumpCountRange(PRM_RANGE_RESTRICTED, 1, PRM_RANGE_UI, 100);

// TWIST OPERATION PARAMETERS
static PRM_Name twistAmountName("twist_amount", "Twist Amount");
static PRM_Range twistAmountRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 360.0f);

static PRM_Name twistFrequencyName("twist_frequency", "Twist Frequency");
static PRM_Range twistFrequencyRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 10.0f);

// NOISE OPERATION PARAMETERS
static PRM_Name noiseFrequencyName("noise_frequency", "Noise Frequency");
static PRM_Range noiseFrequencyRange(PRM_RANGE_RESTRICTED, 0.1f, PRM_RANGE_UI, 10.0f);

static PRM_Name noiseAmplitudeName("noise_amplitude", "Noise Amplitude");
static PRM_Range noiseAmplitudeRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 1.0f);

static PRM_Name noiseScaleName("noise_scale", "Noise Scale");
static PRM_Range noiseScaleRange(PRM_RANGE_RESTRICTED, 0.1f, PRM_RANGE_UI, 10.0f);

// BEND OPERATION PARAMETERS
static PRM_Name bendDirXName("bend_dir_x", "Bend Direction X");
static PRM_Range bendDirXRange(PRM_RANGE_RESTRICTED, -1.0f, PRM_RANGE_UI, 1.0f);

static PRM_Name bendDirYName("bend_dir_y", "Bend Direction Y");
static PRM_Range bendDirYRange(PRM_RANGE_RESTRICTED, -1.0f, PRM_RANGE_UI, 1.0f);

static PRM_Name bendDirZName("bend_dir_z", "Bend Direction Z");
static PRM_Range bendDirZRange(PRM_RANGE_RESTRICTED, -1.0f, PRM_RANGE_UI, 1.0f);

static PRM_Name bendMagnitudeName("bend_magnitude", "Bend Magnitude");
static PRM_Range bendMagnitudeRange(PRM_RANGE_RESTRICTED, 0.0f, PRM_RANGE_UI, 1.0f);

PRM_Template
SOP_AuthoringPlugin::myTemplateList[] = {

    // Display parameters
    PRM_Template(PRM_STRING, 1, &strandsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &boundsName, PRMzeroDefaults),
    PRM_Template(PRM_STRING, 1, &statusName, PRMzeroDefaults),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_input", "——— Input ———")),

    // Week 5 - Hair Mesh Loading
    PRM_Template(PRM_FILE, 1, &hairMeshFileName, new PRM_Default(0, "*.obj")),
    PRM_Template(PRM_CALLBACK, 1, &loadHairMeshBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onLoadHairMeshCallback),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_guides", "——— Guides ———")),

    // Change number of guide strands
    PRM_Template(PRM_INT, 1, &numGuidesName, new PRM_Default(20), 0, &numGuidesRange),
    PRM_Template(PRM_CALLBACK, 1, &extractGuidesBtn, nullptr, 0, nullptr, &SOP_AuthoringPlugin::onExtractGuidesCallback),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_viz", "——— Visualization ———")),

    // Week 5 - Cluster Visualization
    PRM_Template(PRM_TOGGLE, 1, &colorByClusterName, new PRM_Default(1)),
    PRM_Template(PRM_INT, 1, &selectedGuideName, new PRM_Default(-1), 0, &selectedGuideRange),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_clump", "——— Clump Operation ———")),

    // Clump parameters
    PRM_Template(PRM_FLT, 1, &clumpRadiusName, new PRM_Default(1.0), 0, &clumpRadiusRange),
    PRM_Template(PRM_FLT, 1, &clumpTightnessName, new PRM_Default(0.5), 0, &clumpTightnessRange),
    PRM_Template(PRM_INT, 1, &clumpCountName, new PRM_Default(20), 0, &clumpCountRange),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_twist", "——— Twist Operation ———")),

    // Twist parameters
    PRM_Template(PRM_FLT, 1, &twistAmountName, new PRM_Default(0.0), 0, &twistAmountRange),
    PRM_Template(PRM_FLT, 1, &twistFrequencyName, new PRM_Default(1.0), 0, &twistFrequencyRange),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_noise", "——— Noise Operation ———")),

    // Noise parameters
    PRM_Template(PRM_FLT, 1, &noiseFrequencyName, new PRM_Default(1.0), 0, &noiseFrequencyRange),
    PRM_Template(PRM_FLT, 1, &noiseAmplitudeName, new PRM_Default(0.0), 0, &noiseAmplitudeRange),
    PRM_Template(PRM_FLT, 1, &noiseScaleName, new PRM_Default(1.0), 0, &noiseScaleRange),

    PRM_Template(PRM_SEPARATOR, 1, new PRM_Name("sep_bend", "——— Bend Operation ———")),

    // Bend parameters
    PRM_Template(PRM_FLT, 1, &bendDirXName, new PRM_Default(0.0), 0, &bendDirXRange),
    PRM_Template(PRM_FLT, 1, &bendDirYName, new PRM_Default(0.0), 0, &bendDirYRange),
    PRM_Template(PRM_FLT, 1, &bendDirZName, new PRM_Default(1.0), 0, &bendDirZRange),
    PRM_Template(PRM_FLT, 1, &bendMagnitudeName, new PRM_Default(0.0), 0, &bendMagnitudeRange),

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

// ============================================================================
// PARAMETER GETTERS & CALLBACKS
// ============================================================================

bool SOP_AuthoringPlugin::getColorByCluster(fpreal t) const {
    return evalInt("color_by_cluster", 0, t);
}

int SOP_AuthoringPlugin::getSelectedGuideIndex(fpreal t) const {
    return evalInt("selected_guide", 0, t);
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

// ============================================================================
// BUILD CLUMP OPERATOR PARAMS FROM UI
// ============================================================================

ClumpOperatorParams SOP_AuthoringPlugin::buildClumpOperatorParams(fpreal t)
{
    ClumpOperatorParams params;

    // Clump parameters
    params.clumpRadius = getClumpRadius(t);
    params.clumpTightness = getClumpTightness(t);
    params.clumpCount = getClumpCount(t);

    // Twist parameters
    params.twistAmount = getTwistAmount(t);
    params.twistFrequency = getTwistFrequency(t);

    // Noise parameters
    params.noiseFrequency = getNoiseFrequency(t);
    params.noiseAmplitude = getNoiseAmplitude(t);
    params.noiseScale = getNoiseScale(t);

    // Bend parameters
    params.bendDirection = UT_Vector3(
        getBendDirectionX(t),
        getBendDirectionY(t),
        getBendDirectionZ(t));
    params.bendMagnitude = getBendMagnitude(t);

    return params;
}

void SOP_AuthoringPlugin::inputConnectChanged(int which_input) {
    SOP_Node::inputConnectChanged(which_input);
}

const char* SOP_AuthoringPlugin::inputLabel(OP_InputIdx idx) const
{
    return "Input";
}

void SOP_AuthoringPlugin::setDisplayStrings(fpreal now, std::string strand_str, std::string bounds_str, std::string status_str) {
    setString(strand_str, CH_STRING_LITERAL, "strand_count", 0, now);
    setString(bounds_str, CH_STRING_LITERAL, "bounding_box", 0, now);
    setString(status_str, CH_STRING_LITERAL, "load_status", 0, now);
}

// ============================================================================
// MAIN COOKING FUNCTION
// ============================================================================

OP_ERROR
SOP_AuthoringPlugin::cookMySop(OP_Context& context)
{
    fpreal now = context.getTime();

    UT_Interrupt* boss = UTgetInterrupt();

    if (boss->opStart("Cooking AuthoringPlugin"))
    {
        gdp->clearAndDestroy();

        // Check if we have loaded strands from file
        if (inputStrands.getStrandCount() == 0)
        {
            statusMessage = "Load a hair mesh file to begin";
            setDisplayStrings(now, "0", "-", statusMessage);
            boss->opEnd();
            return error();
        }

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

        statusMessage = statusBuf;
        addMessage(SOP_MESSAGE, statusBuf);
        setDisplayStrings(now, std::to_string(inputStrands.getStrandCount()), statusBuf, "OK");

        // Display based on state
        if (synthesisReady) {
            // Show all three layers: input, guides, and synthesis
            displayStrandSet(gdp, inputStrands);
            displayGuides(gdp, guides);
            displaySynthesized(gdp, synthesizedStrands);
            addMessage(SOP_MESSAGE, ("Showing " + std::to_string(synthesizedStrands.getStrandCount()) + " synthesized strands").c_str());
        }
        else if (guidesReady) {
            // Show input strands and guides
            displayStrandSet(gdp, inputStrands);
            displayGuides(gdp, guides);
            addMessage(SOP_MESSAGE, "Guides displayed");
        }
        else {
            // Show only input strands
            displayStrandSet(gdp, inputStrands);
        }
    }

    boss->opEnd();
    return error();
}

// ============================================================================
// GUIDE EXTRACTION CALLBACK
// ============================================================================

int SOP_AuthoringPlugin::onExtractGuidesCallback(void* data, int index, fpreal t, const PRM_Template*) {
    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onExtractGuides(t);
    return 1;
}

void SOP_AuthoringPlugin::onExtractGuides(fpreal t) {
    // Task 2 - K-means guide clustering
    std::vector<Feature> features = computeFeatures();
    clusterGuides(getNumGuides(t), features);
    smoothGuides();

    guidesReady = true;
    synthesisReady = false;  // Reset synthesis when guides change
    
    // Automatically trigger synthesis after guides are extracted
    onSynthesizeHair(t);
    
    forceRecook();
}

// ============================================================================
// SYNTHESIS CALLBACK WITH AUTOMATIC RE-SYNTHESIS
// ============================================================================

int SOP_AuthoringPlugin::onSynthesizeHairCallback(void* data, int index, fpreal t, const PRM_Template*) {
    SOP_AuthoringPlugin* sop = static_cast<SOP_AuthoringPlugin*>(data);
    if (!sop) return 0;

    sop->onSynthesizeHair(t);
    return 1;
}

void SOP_AuthoringPlugin::onSynthesizeHair(fpreal t) {
    // Check if guides are ready
    if (guides.getGuideCount() == 0) {
        addMessage(SOP_MESSAGE, "No guides to synthesize from. Extract guides first.");
        return;
    }

    // Build clump operator parameters from current UI values
    ClumpOperatorParams params = buildClumpOperatorParams(t);

    // Apply clump operations (clump → twist → noise → bend)
    synthesizedStrands = ClumpOperator::applyClumpOperations(guides, params);

    if (synthesizedStrands.getStrandCount() == 0) {
        addMessage(SOP_MESSAGE, "Failed to synthesize hair");
        return;
    }

    synthesisReady = true;

    addMessage(SOP_MESSAGE, ("Synthesized " + 
        std::to_string(synthesizedStrands.getStrandCount()) + 
        " strands with clump operations").c_str());

    forceRecook();
}

// ============================================================================
// FEATURE COMPUTATION AND CLUSTERING
// ============================================================================

std::vector<Feature> SOP_AuthoringPlugin::computeFeatures()
{
    if (inputStrands.getStrandCount() == 0)
    {
        statusMessage = "ERROR: No strands loaded for feature computation";
        return std::vector<Feature>();
    }

    std::vector<Feature> features =
        FeatureComputation::computeAllFeatures(inputStrands);

    FeatureComputation::normalizeFeatures(features);

    addMessage(SOP_MESSAGE, "Features computed");

    return features;
}

void SOP_AuthoringPlugin::clusterGuides(int numGuides, std::vector<Feature> features)
{
    addMessage(SOP_MESSAGE, "Begin K-means clustering");

    if (inputStrands.getStrandCount() == 0)
    {
        statusMessage = "ERROR: No strands to cluster";
        return;
    }

    int numStrands = inputStrands.getStrandCount();
    int k = std::min(numGuides, numStrands);
    int featureDim = features[0].values.size();

    std::vector<int> centerIndices;
    std::srand(67);

    centerIndices.push_back(std::rand() % numStrands);

    for (int c = 1; c < k; c++) {

        std::vector<float> minDists(numStrands, FLT_MAX);
        for (int i = 0; i < numStrands; ++i) {
            for (int ci : centerIndices) {
                float dist = features[i].featureDistance(features[ci]);
                minDists[i] = std::min(minDists[i], dist);
            }
        }

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

    std::vector<std::array<float, 7>> centers(k);
    for (int c = 0; c < k; ++c)
        centers[c] = features[centerIndices[c]].values;

    std::vector<int> assignments(numStrands, -1);
    const int maxIterations = 100;

    for (int iter = 0; iter < maxIterations; ++iter)
    {
        bool changed = false;

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

        if (!changed)
        {
            addMessage(SOP_MESSAGE, ("K-means converged at iteration " +
                std::to_string(iter)).c_str());
            break;
        }

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
                newCenters[c] = features[std::rand() % numStrands].values;
            }
        }

        centers = newCenters;
    }

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

    ClusterVisualization::assignClustersToStrands(inputStrands, guides);

    addMessage(SOP_MESSAGE, "K-means clustering complete");
    addMessage(SOP_MESSAGE, std::to_string(guides.getGuideCount()).c_str());
}

void SOP_AuthoringPlugin::smoothGuides()
{
    if (guides.getGuideCount() == 0)
    {
        statusMessage = "ERROR: No guides to smooth";
        return;
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
// DISPLAY FUNCTIONS
// ============================================================================

void SOP_AuthoringPlugin::displayStrandSet(GU_Detail* gdp, const StrandSet& strands)
{
    if (!gdp) return;

    bool colorByCluster = getColorByCluster(0);
    int selectedCluster = getSelectedGuideIndex(0);

    std::vector<UT_Vector3> clusterColors;
    if (colorByCluster && guides.getGuideCount() > 0) {
        clusterColors = ClusterVisualization::generateClusterColors(guides.getGuideCount());
    }

    GA_RWHandleV3 colorHandle(gdp->addDiffuseAttribute(GA_ATTRIB_PRIMITIVE));

    for (int i = 0; i < strands.getStrandCount(); ++i) {
        const Strand& strand = strands.getStrand(i);
        if (strand.positions.size() < 2) continue;

        UT_Vector3 strandColor;
        float brightness = 1.0f;

        if (colorByCluster && guides.getGuideCount() > 0) {
            strandColor = ClusterVisualization::getClusterColor(
                strand.clusterID,
                guides.getGuideCount());

            brightness = ClusterVisualization::getHighlightFactor(
                strand.clusterID,
                selectedCluster,
                selectedCluster >= 0);

            strandColor *= brightness;
        }
        else {
            strandColor = UT_Vector3(1.0f, 1.0f, 1.0f);
        }

        GA_Offset startPt = gdp->appendPointBlock(strand.positions.size());
        for (int p = 0; p < (int)strand.positions.size(); ++p)
            gdp->setPos3(startPt + p, strand.positions[p]);

        GEO_PrimPoly* prim = GEO_PrimPoly::build(gdp, strand.positions.size(), GU_POLY_OPEN, false);
        for (int p = 0; p < (int)strand.positions.size(); ++p)
            prim->setPointOffset(p, startPt + p);

        if (colorHandle.isValid())
            colorHandle.set(prim->getMapOffset(), strandColor);
    }
}

void SOP_AuthoringPlugin::displayGuides(GU_Detail* gdp, const GuideSet& guides)
{
    if (!gdp) return;

    GA_RWHandleV3 colorHandle(gdp->addDiffuseAttribute(GA_ATTRIB_PRIMITIVE));

    for (int i = 0; i < guides.getGuideCount(); i++) {
        const Strand& guide = guides.getGuide(i);
        if (guide.positions.size() < 2) continue;

        GA_Offset startPt = gdp->appendPointBlock(guide.positions.size());
        for (int p = 0; p < (int)guide.positions.size(); p++)
            gdp->setPos3(startPt + p, guide.positions[p]);

        GEO_PrimPoly* prim = GEO_PrimPoly::build(gdp, guide.positions.size(), GU_POLY_OPEN, false);
        for (int p = 0; p < (int)guide.positions.size(); p++)
            prim->setPointOffset(p, startPt + p);

        if (colorHandle.isValid())
            colorHandle.set(prim->getMapOffset(), UT_Vector3(1.0f, 0.0f, 0.0f));
    }
}

void SOP_AuthoringPlugin::displaySynthesized(GU_Detail* gdp, const StrandSet& strands)
{
    if (!gdp) return;

    GA_RWHandleV3 colorHandle(gdp->addDiffuseAttribute(GA_ATTRIB_PRIMITIVE));

    for (int i = 0; i < strands.getStrandCount(); ++i) {
        const Strand& strand = strands.getStrand(i);
        if (strand.positions.size() < 2) continue;

        GA_Offset startPt = gdp->appendPointBlock(strand.positions.size());
        for (int p = 0; p < (int)strand.positions.size(); ++p)
            gdp->setPos3(startPt + p, strand.positions[p]);

        GEO_PrimPoly* prim = GEO_PrimPoly::build(gdp, strand.positions.size(), GU_POLY_OPEN, false);
        for (int p = 0; p < (int)strand.positions.size(); ++p)
            prim->setPointOffset(p, startPt + p);

        if (colorHandle.isValid())
            colorHandle.set(prim->getMapOffset(), UT_Vector3(0.0f, 1.0f, 0.0f));
    }
}