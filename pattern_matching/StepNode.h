#pragma once
#include "StepComponent.h"
#include <STEPfile.h>

class StepNode : public StepComponent
{
public:
	StepNode(SDAI_Application_instance*& inst);
	virtual ~StepNode();

private:
	SDAI_Application_instance* _inst;
};

