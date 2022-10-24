#include <STEPfile.h>
#include <sdai.h>
#include <STEPattribute.h>
#include <ExpDict.h>
#include <Registry.h>

#include <regex>


#include "schema.h"

#include <yaml-cpp/yaml.h>

#include "StepComponent.h"
#include "StepComposite.h"
#include "StepDerivedNode.h"
#include "StepNode.h"

void PrintDebugMessage(int id, STEPattribute* attribute, std::stringstream& debug_log, int loop_count)
{
	for (int p = 0; p < loop_count; p++)
	{
		debug_log << "  ";
	}

	if( !attribute ) 
	{
		debug_log << id << ":node" << std::endl;
		return;
	}

	auto attr_select = attribute->Select();
	auto attr_instance = attribute->Entity();
	auto attr_aggr = attribute->Aggregate();

	if (!attr_select && !attr_instance && !attr_aggr)
	{
		debug_log << id << ":" << attribute->Name() << ":" << attribute->asStr() << std::endl;
		return;
	}

	if (attribute->IsDerived())
	{
		debug_log << id << ":derived(" << attribute->Name() << ")" << std::endl;
	}
	else if (attr_aggr != nullptr)
	{
		debug_log << id << ":aggregate(" << attribute->Name() << ")" << std::endl;
	}
	else if (attr_select != nullptr)
	{
		debug_log << id << ":selector(" << attribute->Name() << ")" << std::endl;
	}
	else if (attr_instance != nullptr)
	{
		debug_log << id << ":instance(" << attribute->Name() << ")" << std::endl;
	}
}

void AddNode(InstMgr*& inst_mgr, StepComponent* base_component, SDAI_Application_instance* const& instance, std::stringstream& debug_log, YAML::Node& yaml_node, int loop_count)
{
	int id = instance->GetFileId();

	for (int j = 0; j < instance->AttributeCount(); j++)
	{
		auto attribute = &instance->attributes[j];

		auto attr_select = attribute->Select();
		auto attr_instance = attribute->Entity();
		auto attr_aggr = attribute->Aggregate();

		PrintDebugMessage(id, attribute, debug_log, loop_count);

		if (!attr_select && !attr_instance && !attr_aggr)
		{
			StepComponent* simple_node = new StepNode(instance);
			base_component->AddComponent(simple_node);

			yaml_node[attribute->Name()] = attribute->asStr();
			continue;
		}

		if (attribute->IsDerived())
		{
			StepComponent* derived_node = new StepDerivedNode(instance);
			base_component->AddComponent(derived_node);

			yaml_node[attribute->Name()] = attribute->asStr();
		}
		else if(attr_aggr != nullptr)
		{
			auto head_node = attr_aggr->GetHead();

			std::vector<string> sub_node;
			std::map<std::string, YAML::Node> named_map;

			for (auto node = head_node; node != nullptr; node = node->next)
			{
				auto conv_node = dynamic_cast<const EntityNode*>(node);
				auto conv_raw_node = dynamic_cast<STEPnode*>(node);

				if(conv_node != nullptr) 
				{
					auto inst = conv_node->node;

					StepComponent* child_node = new StepComposite(inst);
					base_component->AddComponent(child_node);

					YAML::Node yaml_child_node;
					AddNode(inst_mgr, child_node, inst, debug_log, yaml_child_node, loop_count + 1);

					std::stringstream ss_str;
					ss_str << "#" << inst->GetFileId();
					named_map[ss_str.str()] = yaml_child_node;
				}
				else if(conv_raw_node != nullptr) 
				{
					PrintDebugMessage(id, nullptr, debug_log, loop_count + 1);

					std::string str;
					conv_raw_node->asStr(str);
					sub_node.push_back(str);
				}
				else 
				{
					PrintDebugMessage(id, nullptr, debug_log, loop_count + 1);
				}
			}

			if(named_map.size() != 0 ) 
			{
				yaml_node[attribute->Name()] = named_map;
			}
			else if(sub_node.size() != 0) 
			{
				yaml_node[attribute->Name()] = sub_node;
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

				YAML::Node yaml_child_node;
				AddNode(inst_mgr, child_node, select_instance, debug_log, yaml_child_node, loop_count + 1);

				yaml_node[attribute->Name()] = yaml_child_node;

			}
		}
		else if(attr_instance != nullptr)
		{
			StepComponent* child_node = new StepComposite(attr_instance);
			base_component->AddComponent(child_node);

			YAML::Node yaml_child_node;
			AddNode(inst_mgr, child_node, attr_instance, debug_log, yaml_child_node, loop_count + 1);

			yaml_node[attribute->Name()] = yaml_child_node;
		}
	}
}


int main( int argv, char** argc)
{
	std::string path = ".\\StepData\\BSP35B20-N-12.stp";
	if(argv >= 2) 
	{
		path = argc[1];
	}

	// The registry contains information about types present in the current schema; SchemaInit is a function in the schema-specific SDAI library
	Registry* registry = new Registry(SchemaInit);

	// The InstMgr holds instances that have been created or that have been loaded from a file
	InstMgr* instance_list = new InstMgr();

	// Increment FileId so entities start at #1 instead of #0.
	instance_list->NextFileId();

	// STEPfile takes care of reading and writing Part 21 files
	STEPfile* sfile = new STEPfile(*registry, *instance_list, path, false);

	std::stringstream debug_log;
	YAML::Node root_node;
	auto root_component = new StepComposite();
	for(int i = 0 ; i < instance_list->InstanceCount(); i++ ) 
	{
		auto instance = instance_list->GetSTEPentity(i);
		int file_id = instance->GetFileId();

		if (!root_component->ContainFileId(file_id))
		{

			std::cout << "#" << file_id << "export" << std::endl;
			debug_log << file_id << ":root(" << instance->EntityName() << ")" << std::endl;

			StepComponent* base_node = new StepComposite(instance);
			root_component->AddComponent(base_node);

			YAML::Node step_node;
			AddNode(instance_list, base_node, instance, debug_log, step_node, 1);

			std::stringstream ss_str;
			ss_str << "#" << instance->GetFileId();
			root_node[ss_str.str()] = step_node;
		}
	}

	std::ofstream ofs("result.txt");
	if(!ofs) 
	{
		return 2;
	}
	ofs << debug_log.str();
	ofs.close();

	std::ofstream ofs_yaml("result.yaml");
	if(!ofs_yaml) 
	{
		return 2;
	}
	YAML::Emitter yaml_emitter(ofs_yaml);
	yaml_emitter << root_node;
	ofs_yaml.close();

	return 0;
}
