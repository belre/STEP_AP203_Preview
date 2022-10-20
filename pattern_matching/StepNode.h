#pragma once
#include "StepComponent.h"
#include <STEPfile.h>

class StepNode : public StepComponent
{
public:
	StepNode();
	virtual ~StepNode();

	bool ContainFileId(int id) override;

};

