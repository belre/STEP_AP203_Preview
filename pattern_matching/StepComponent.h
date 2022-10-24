#pragma once

#include <yaml-cpp/yaml.h>

class StepComponent
{
public:
	static YAML::Node InvalidNode;

public:
	StepComponent();
	virtual ~StepComponent();

	virtual bool ContainFileId(int id);

	virtual YAML::Node& GetYamlNode()
	{
		return InvalidNode;
	}
	
	virtual void AddComponent(StepComponent*&)
	{
		
	}
};

