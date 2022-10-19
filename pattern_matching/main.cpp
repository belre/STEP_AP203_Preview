#include <STEPfile.h>
#include <sdai.h>
#include <STEPattribute.h>
#include <ExpDict.h>
#include <Registry.h>
#include <errordesc.h>

#include <STEPcomplex.h>
#include <SdaiHeaderSchema.h>

#include "schema.h"

#include <SdaiCONFIG_CONTROL_DESIGN.h>

#include "StepComponent.h"
#include "StepComposite.h"
#include "StepDerivedNode.h"
#include "StepNode.h"

void AddNode(StepComposite* root_component, SDAI_Application_instance* const& instance, int loop_count)
{
	int id = instance->GetFileId();


	for (int j = 0; j < instance->AttributeCount(); j++)
	{
		auto attribute = instance->attributes[j];

		auto type = attribute.BaseType();


		for (int i = 0; i < loop_count; i++)
		{
			std::cout << "  ";
		}

		if ((type & BASE_TYPE::sdaiINSTANCE) == 0)
		{
			std::cout << id  << ":node" << std::endl;
			continue;
		}

		auto attr_instance = attribute.Entity();
		auto attr_aggr = attribute.Aggregate();
		if (attr_instance == nullptr && attr_aggr == nullptr)
		{
			std::cout << id  << ":node" << std::endl;
			continue;
		}


		if (attribute.IsDerived())
		{
			StepComponent* derived_node = new StepDerivedNode();
			root_component->AddComponent(derived_node);

			std::cout << id << "," << ":derived" << std::endl;
		}
		else if(attr_aggr != nullptr)
		{
			auto head_node = dynamic_cast<const EntityNode*>(attr_aggr->GetHead());

			std::cout << id  << ":aggregate" << std::endl;

			for(auto node = head_node; node != nullptr; node = dynamic_cast<const EntityNode*>(node->next))
			{
				AddNode(root_component, node->node, loop_count + 1);
			}
		}
		else if(attr_instance != nullptr)
		{
			StepComponent* child_node = new StepNode(attr_instance);
			root_component->AddComponent(child_node);

			std::cout << id << "," << attr_instance->GetFileId() << ":instance" << std::endl;
			AddNode(root_component, attr_instance, loop_count + 1);
		}
	}
}


int main( int argv, char** argc)
{
	// The registry contains information about types present in the current schema; SchemaInit is a function in the schema-specific SDAI library
	Registry* registry = new Registry(SchemaInit);

	// The InstMgr holds instances that have been created or that have been loaded from a file
	InstMgr* instance_list = new InstMgr();

	// Increment FileId so entities start at #1 instead of #0.
	instance_list->NextFileId();

	// STEPfile takes care of reading and writing Part 21 files
	STEPfile* sfile = new STEPfile(*registry, *instance_list, ".\\StepData\\BSP35B20-N-12.stp", false);

	auto root_component = new StepComposite();
	for(int i = 0 ; i < instance_list->InstanceCount(); i++ ) 
	{
		auto instance = instance_list->GetSTEPentity(i);

		std::cout << i << "Extraction" << std::endl;
		AddNode(root_component, instance, 1);
	}

	return 0;
}
