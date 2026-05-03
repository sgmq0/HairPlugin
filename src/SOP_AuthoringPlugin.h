#ifndef __SOP_AUTHORINGPLUGIN_H__
#define __SOP_AUTHORINGPLUGIN_H__

#include <SOP/SOP_Node.h>
#include "StrandSet.h"
#include "GuideSet.h"
#include "GeometryImporter.h"
#include "FeatureComputation.h"
#include "GuideSmoothing.h"
#include "ClosestGuides.h"
#include "MeshToStrands.h"
#include "ClusterVisualization.h"
#include "ClumpOperator.h"
#include "LossComputation.h"
#include "SimpleOptimizer.h"
#include "ExportManager.h"

class SOP_AuthoringPlugin : public SOP_Node
{
public:
    static OP_Node* myConstructor(OP_Network*, const char*,
        OP_Operator*);
    static PRM_Template myTemplateList[];

protected:
    SOP_AuthoringPlugin(OP_Network* net, const char* name, OP_Operator* op);
    virtual ~SOP_AuthoringPlugin();

    virtual unsigned disableParms();
    virtual OP_ERROR cookMySop(OP_Context& context);

    // callback for whenever an input is connected/disconnected
    void inputConnectChanged(int which_input) override;

    // override to give the inputs labels
    const char* inputLabel(OP_InputIdx idx) const override;

public:
    int getNumGuides(fpreal t = 0) const
    {
        return evalInt("num_guides", 0, t);
    }

    // Scale operator
    float getScaleFactor(fpreal t = 0) const
    {
        return evalFloat("scale_factor", 0, t);
    }

    // clump operator
    float getClumpProfile(fpreal t = 0) const
    {
        return evalFloat("clump_profile", 0, t);
    }

    // bend operator
    float getBendAngle(fpreal t = 0) const
    {
        return evalFloat("bend_angle", 0, t);
    }

    float getBendStart(fpreal t = 0) const
    {
        return evalFloat("bend_start", 0, t);
    }

    // curl operator
    float getCurlRadius(fpreal t = 0) const
    {
        return evalFloat("curl_radius", 0, t);
    }

    float getCurlFrequency(fpreal t = 0) const
    {
        return evalFloat("curl_frequency", 0, t);
    }

    float getCurlRandomFrequency(fpreal t = 0) const
    {
        return evalFloat("curl_random_frequency", 0, t);
    }

    float getCurlStart(fpreal t = 0) const
    {
        return evalFloat("curl_start", 0, t);
    }

    // Noise operator
    float getNoiseAmplitude(fpreal t = 0) const
    {
        return evalFloat("noise_amplitude", 0, t);
    }

    float getNoiseFrequency(fpreal t = 0) const
    {
        return evalFloat("noise_frequency", 0, t);
    }

    // Week 5 - Cluster Visualization getters
    bool getColorByCluster(fpreal t = 0) const {
        return evalInt("color_by_cluster", 0, t) != 0;
    }

    int getSelectedGuideIndex(fpreal t = 0) const {
        return evalInt("selected_guide", 0, t);
    }

    // show/hide scalp
    bool getShowScalpEnabled(fpreal t = 0) const {
        return evalInt("opt_show_scalp", 0, t) != 0;
    }

    const char* getStatusMessage() const { return statusMessage.c_str(); }

private:
    // display functions
    void displayStrandSet(GU_Detail* gdp, const StrandSet& strands, bool useOriginal);
    void displayGuides(GU_Detail* gdp, const GuideSet& guides);
    void setDisplayStrings(fpreal now, std::string strand_str, std::string bounds_str, std::string status_str);

    // Week 5 - Mesh Loading callbacks. extract guide strands when clicked
    static int onLoadHairMeshCallback(void* data, int index, fpreal t, const PRM_Template*);
    void onLoadHairMesh(fpreal t);

    // Task 2 - Guide extraction callback. 
    static int onExtractGuidesCallback(void* data, int index, fpreal t, const PRM_Template*);
    void onExtractGuides(fpreal t);

    // Task 3 - Synthesis callbacks.
    static int onSynthesizeHairCallback(void* data, int index, fpreal t, const PRM_Template*);
    void onSynthesizeHair(fpreal t);

    // Task 4 - Optimization callback
    static int onOptimizeCallback(void* data, int index, fpreal t, const PRM_Template*);
    void onOptimize(fpreal t);

    // Task 5 - Export callback
    static int onExportCallback(void* data, int index, fpreal t, const PRM_Template*);
    void onExport(fpreal t);

    // Feature computation and clustering
    std::vector<Feature> computeFeatures();                // step 2.1: feature vector computation
    void clusterGuides(int numGuides, std::vector<Feature> features);   // step 2.2: k-means clustering

    // various helper/utility functions
    void smoothGuides();    // step 2.3: guide smoothing.
    void synthesizeHair(GU_Detail* gdp);
    void extractRootsFromInputStrands();    // runs when synthesize button is clicked

    // Member variables 
    StrandSet inputStrands;         // input curves
    GuideSet guides;                // guides generated with onExtractGuides
    StrandSet synthesizedStrands;   // strands synthesized with onSynthesizeHair
    std::vector<UT_Vector3> strandRoots;    // vector storing all the root positions of input curves. 
    ClosestGuides closestGuides;
    std::string statusMessage;
    ClumpParams hairParams;

    // Flags
    bool inputLoaded = false;
    bool guidesReady = false;
    bool synthesisReady = false;
    bool scalpConnected = false;
    bool needFirstSynthesis = false;

    // Track parameter changes for auto-resynthesis
    float cachedScaleFactor = -1.0f;
    float cachedClumpProfile = -1.0f;
    float cachedBendAngle = -1.0f;
    float cachedBendStart = -1.0f;
    float cachedCurlRadius = -1.0f;
    float cachedCurlFrequency = -1.0f;
    float cachedCurlRandomFrequency = -1.0f;
    float cachedCurlStart = 1.0f;

    float cachedNoiseAmplitude = -1.0f;
    float cachedNoiseFrequency = -1.0f;
};

#endif