#include <UT/UT_DSOVersion.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>

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

PRM_Template
SOP_AuthoringPlugin::myTemplateList[] = {
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

OP_ERROR
SOP_AuthoringPlugin::cookMySop(OP_Context& context)
{
    fpreal now = context.getTime();

    UT_Interrupt* boss = UTgetInterrupt();

    if (boss->opStart("Cooking AuthoringPlugin"))
    {
        gdp->clearAndDestroy();
        // TODO: Add actual cooking logic here
    }

    boss->opEnd();
    return error();
}

void SOP_AuthoringPlugin::loadGeometry(const GU_Detail* input_geo)
{
}

void SOP_AuthoringPlugin::computeFeatures()
{
}

void SOP_AuthoringPlugin::clusterGuides(int numGuides)
{
}

void SOP_AuthoringPlugin::smoothGuides()
{
}

void SOP_AuthoringPlugin::synthesizeHair()
{
}

void SOP_AuthoringPlugin::displayStrandSet(GU_Detail* gdp,
    const StrandSet& strands)
{
}

void SOP_AuthoringPlugin::displayGuides(GU_Detail* gdp,
    const GuideSet& guides)
{
}

void SOP_AuthoringPlugin::displaySynthesized(GU_Detail* gdp,
    const StrandSet& synthesized)
{
}