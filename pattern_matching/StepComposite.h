#pragma once

#include <vector>
#include "StepComponent.h"

#include <STEPfile.h>

class StepComposite : public StepComponent
{
public:
	StepComposite();
	StepComposite(SDAI_Application_instance*& inst) ;
	virtual ~StepComposite();

public:

	bool ContainFileId(int id) override;

	int GetFileId() const
	{
		if(_inst != nullptr) 
		{
			return _inst->GetFileId();
		}
		else 
		{
			return -1;
		}
	}

	void AddComponent(StepComponent*&) override;

private:
	std::vector<StepComponent*> _component;
	SDAI_Application_instance* _inst;
};

