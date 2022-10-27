#pragma once
#include "StepComponent.h"

class SDAI_Application_instance;

class StepDerivedNode : public StepComponent
{
public:
	StepDerivedNode(SDAI_Application_instance* const& inst);
	virtual ~StepDerivedNode();


	YAML::Node& GetYamlNode() override
	{
		return InvalidNode;
	}

private:
	SDAI_Application_instance* _inst;
};

