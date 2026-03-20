#ifndef __SOP_AUTHORINGPLUGIN_H__
#define __SOP_AUTHORINGPLUGIN_H__

#include <SOP/SOP_Node.h>
#include "StrandSet.h"
#include "GuideSet.h"
#include "GeometryImporter.h"

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

public:
    int getNumGuides(fpreal t = 0) const
    {
        return evalInt("num_guides", 0, t);
    }
    const char* getStatusMessage() const { return statusMessage.c_str(); }

private:
    void loadGeometry(const GU_Detail* input_geo);
    void computeFeatures();
    void clusterGuides(int numGuides);
    void smoothGuides();
    void synthesizeHair();
    void displayStrandSet(GU_Detail* gdp, const StrandSet& strands);
    void displayGuides(GU_Detail* gdp, const GuideSet& guides);
    void displaySynthesized(GU_Detail* gdp, const StrandSet& synthesized);

    StrandSet inputStrands;
    GuideSet guides;
    StrandSet synthesizedStrands;
    std::string statusMessage;
};

#endif