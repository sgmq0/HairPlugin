#ifndef __SOP_AUTHORINGPLUGIN_H__
#define __SOP_AUTHORINGPLUGIN_H__

#include <SOP/SOP_Node.h>
#include "StrandSet.h"
#include "GuideSet.h"
#include "GeometryImporter.h"
#include "FeatureComputation.h"
#include "GuideSmoothing.h"
#include "ClosestGuides.h"

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

public:
    int getNumGuides(fpreal t = 0) const
    {
        return evalInt("num_guides", 0, t);
    }
    const char* getStatusMessage() const { return statusMessage.c_str(); }

private:
    void displayStrandSet(GU_Detail* gdp, const StrandSet& strands);
    void displayGuides(GU_Detail* gdp, const GuideSet& guides);
    void displaySynthesized(GU_Detail* gdp, const StrandSet& synthesized);

    void setDisplayStrings(fpreal now, std::string strand_str, std::string bounds_str, std::string status_str);

    // function that does all of step 2
    static int onExtractGuidesCallback(void* data, int index, fpreal t, const PRM_Template*);
    void onExtractGuides(fpreal t);
    std::vector<Feature> computeFeatures();
    void clusterGuides(int numGuides, std::vector<Feature> features);
    void smoothGuides();
    void synthesizeHair(fpreal t);

    StrandSet inputStrands;
    GuideSet guides;
    StrandSet synthesizedStrands;
    std::string statusMessage;

    ClosestGuides closestGuides;

    // flags
    bool guidesReady = false;
};

#endif