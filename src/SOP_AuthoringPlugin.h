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

    float getClumpRadius(fpreal t = 0) const
    {
        return evalFloat("clump_radius", 0, t);
    }

    float getClumpTightness(fpreal t = 0) const
    {
        return evalFloat("clump_tightness", 0, t);
    }

    int getClumpCount(fpreal t = 0) const
    {
        return evalInt("clump_count", 0, t);
    }

    // Week 5 - Cluster Visualization getters
    bool getColorByCluster(fpreal t = 0) const;

    int getSelectedGuideIndex(fpreal t = 0) const;

    const char* getStatusMessage() const { return statusMessage.c_str(); }

private:
    // display functions
    void displayStrandSet(GU_Detail* gdp, const StrandSet& strands);
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

    // Feature computation and clustering
    std::vector<Feature> computeFeatures();                // step 2.1: feature vector computation
    void clusterGuides(int numGuides, std::vector<Feature> features);   // step 2.2: k-means clustering

    // various helper/utility functions
    void smoothGuides();    // step 2.3: guide smoothing.
    void synthesizeHair();  
    void extractRootsFromInputStrands();    // runs when synthesize button is clicked

    // Member variables 
    StrandSet inputStrands;         // input curves
    GuideSet guides;                // guides generated with onExtractGuides
    StrandSet synthesizedStrands;   // strands synthesized with onSynthesizeHair
    std::vector<UT_Vector3> strandRoots;    // vector storing all the root positions of input curves. 
    ClosestGuides closestGuides;
    std::string statusMessage;

    // Flags
    bool inputLoaded = false;
    bool guidesReady = false;
    bool synthesisReady = false;
    bool scalpConnected = false;
};

#endif