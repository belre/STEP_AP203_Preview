#pragma once

class StepComponent
{
public:
	StepComponent();
	virtual ~StepComponent();

	virtual bool ContainFileId(int id);

	virtual void AddComponent(StepComponent*&)
	{
		
	}
};

