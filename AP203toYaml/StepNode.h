#pragma once
#include "StepComponent.h"
#include <STEPfile.h>

class StepNode : public StepComponent
{
public:
	StepNode(SDAI_Application_instance* const& inst);
	virtual ~StepNode();

	bool ContainFileId(int id) override;

	YAML::Node& GetYamlNode() override
	{
		return InvalidNode;
	}

private:
	SDAI_Application_instance* _inst;
};

