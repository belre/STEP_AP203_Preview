#include "StepComposite.h"

StepComposite::StepComposite()
{
	_inst = nullptr;
}

StepComposite::StepComposite(SDAI_Application_instance*& inst)
{
	_inst = inst;
}

void StepComposite::AddComponent(StepComponent*& component)
{
	_component.push_back(component);
}


StepComposite::~StepComposite()
{
	
}
