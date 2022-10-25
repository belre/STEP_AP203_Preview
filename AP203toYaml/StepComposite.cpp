#include "StepComposite.h"

StepComposite::StepComposite()
{
	_inst = nullptr;
}

StepComposite::StepComposite(SDAI_Application_instance*& inst)
{
	_inst = inst;
	_component.resize(0);
}



void StepComposite::AddComponent(StepComponent*& component, bool is_complex)
{
	_component.push_back(component);

	_is_complex = is_complex;
}

bool StepComposite::ContainFileId(int id)
{
	if(_inst != nullptr && _inst->GetFileId() == id) 
	{
		return true;
	}
	
	for(auto iter = _component.begin(); iter != _component.end(); iter++) 
	{
		if((*iter) != nullptr && (*iter)->ContainFileId(id))
		{
			return true;
		}
	}

	return false;
}


StepComposite::~StepComposite()
{
	
}
