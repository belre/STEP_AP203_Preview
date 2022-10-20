#include <STEPfile.h>
#include <sdai.h>
#include <STEPattribute.h>
#include <ExpDict.h>
#include <Registry.h>
#include <errordesc.h>
#include <regex>

#include <STEPcomplex.h>
#include <SdaiHeaderSchema.h>

#include "schema.h"

#include <SdaiCONFIG_CONTROL_DESIGN.h>

#include "StepComponent.h"
#include "StepComposite.h"
#include "StepDerivedNode.h"
#include "StepNode.h"

void AddNode(InstMgr*& inst_mgr, StepComponent* base_component, SDAI_Application_instance* const& instance, int loop_count)
{
	int id = instance->GetFileId();

	for (int j = 0; j < instance->AttributeCount(); j++)
	{
		auto attribute = &instance->attributes[j];

		auto type = attribute->BaseType();

		auto attr_select = attribute->Select();
		auto attr_instance = attribute->Entity();
		auto attr_aggr = attribute->Aggregate();

		if (!attr_select && !attr_instance && !attr_aggr)
		{
			StepComponent* simple_node = new StepNode();
			base_component->AddComponent(simple_node);

			continue;
		}


		if (attribute->IsDerived())
		{
			StepComponent* derived_node = new StepDerivedNode();
			base_component->AddComponent(derived_node);

			//std::cout << id << ":derived(" << attribute->Name() << "):" << std::endl;
		}
		else if(attr_aggr != nullptr)
		{
			auto head_node = dynamic_cast<const EntityNode*>(attr_aggr->GetHead());

			for(auto node = head_node; node != nullptr; node = dynamic_cast<const EntityNode*>(node->next))
			{
				auto inst = node->node;

				StepComponent* child_node = new StepComposite(inst);
				base_component->AddComponent(child_node);

				AddNode(inst_mgr, child_node, node->node, loop_count + 1);
			}
		}
		else if(attr_select != nullptr) 
		{
			auto type = attr_select->ValueType();

			if((type & sdaiINSTANCE) != 0 ) 
			{
				auto aDesc = attribute->getADesc();
				auto dd = attribute->asStr();

				std::string out_text;
				attr_select->STEPwrite(out_text);

				// ap203 only?
				std::regex id_detect_regex("\\#(\\d+)");
				std::smatch match;
				if(!std::regex_search(out_text, match, id_detect_regex) || 
 					match.length() < 2)
				{
					std::cout << "Regex Pattern Match failed" << std::endl;
					continue;
				}

				int id = 0;
				try 
				{
					id = std::stol(match[1].str());
				}
				catch(exception) 
				{
					std::cout << "Regex ID error" << std::endl;
					continue;
				}

				auto select_entity = inst_mgr->FindFileId(id);
				if(select_entity == nullptr) 
				{
					std::cout << "Regex Conversion error" << std::endl;
					continue;
				}

				auto select_instance = select_entity->GetApplication_instance();
				StepComponent* child_node = new StepComposite(select_instance);
				base_component->AddComponent(child_node);

				AddNode(inst_mgr, child_node, select_instance, loop_count + 1);
			}
		}
		else if(attr_instance != nullptr)
		{
			StepComponent* child_node = new StepComposite(attr_instance);
			base_component->AddComponent(child_node);

			AddNode(inst_mgr, child_node, attr_instance, loop_count + 1);
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
		int file_id = instance->GetFileId();

		if(!root_component->ContainFileId(file_id) )
		{
			std::cout << file_id << "Extraction" << std::endl;

			StepComponent* base_node = new StepComposite(instance);
			root_component->AddComponent(base_node);

			AddNode(instance_list, base_node, instance, 1);
		}
	}

	return 0;
}
