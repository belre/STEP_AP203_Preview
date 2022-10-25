#include <STEPfile.h>
#include <sdai.h>
#include <STEPattribute.h>
#include <ExpDict.h>
#include <Registry.h>

#include <regex>
#include <STEPaggrEntity.h>
#include <unordered_set>


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

	if (!attribute)
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

		yaml_node["sc_fileid"] = id;
		yaml_node["sc_function"] = instance->EntityName();

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
		else if (attr_aggr != nullptr)
		{
			auto head_node = attr_aggr->GetHead();

			std::vector<std::string> sub_node;
			std::vector<YAML::Node> aggr_node;
			std::map<std::string, YAML::Node> named_map;

			for (auto node = head_node; node != nullptr; node = node->next)
			{
				auto conv_node = dynamic_cast<const EntityNode*>(node);
				auto conv_raw_node = dynamic_cast<STEPnode*>(node);

				if (conv_node != nullptr)
				{
					auto inst = conv_node->node;

					StepComponent* child_node = new StepComposite(inst);
					base_component->AddComponent(child_node);

					YAML::Node yaml_child_node;
					AddNode(inst_mgr, child_node, inst, debug_log, yaml_child_node, loop_count + 1);

					aggr_node.push_back(yaml_child_node);

					std::stringstream ss_str;
					ss_str << "#" << inst->GetFileId();
					named_map[ss_str.str()] = yaml_child_node;
				}
				else if (conv_raw_node != nullptr)
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

			if (aggr_node.size() != 0)
			{
				yaml_node[attribute->Name()] = aggr_node;
			}
			else if (sub_node.size() != 0)
			{
				yaml_node[attribute->Name()] = sub_node;
			}
		}
		else if (attr_select != nullptr)
		{
			auto type = attr_select->ValueType();

			if ((type & sdaiINSTANCE) != 0)
			{

				std::string out_text;
				attr_select->STEPwrite(out_text);

				// ap203 only?
				std::regex id_detect_regex("\\#(\\d+)");
				std::smatch match;
				if (!std::regex_search(out_text, match, id_detect_regex) ||
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
				catch (exception)
				{
					std::cout << "Regex ID error" << std::endl;
					continue;
				}

				auto select_entity = inst_mgr->FindFileId(id);
				if (select_entity == nullptr)
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
		else if (attr_instance != nullptr)
		{
			StepComponent* child_node = new StepComposite(attr_instance);
			base_component->AddComponent(child_node);

			YAML::Node yaml_child_node;
			AddNode(inst_mgr, child_node, attr_instance, debug_log, yaml_child_node, loop_count + 1);

			yaml_node[attribute->Name()] = yaml_child_node;
		}
	}
}

void CountChildNodeId(YAML::Node& node, std::vector<int>& id_stock)
{
	if(node.IsSequence())
	{
		for (auto tmp = node.begin(); tmp != node.end(); ++tmp)
		{
			CountChildNodeId(*tmp, id_stock);
		}
	}
	else if(node.IsMap()) 
	{
		if( node["complex_element"] ) 
		{
			auto complex_node = node["complex_element"];
			CountChildNodeId(complex_node, id_stock);
			return;
		}

		if (!node["sc_fileid"])
		{
			return;
		}

		int file_id = node["sc_fileid"].as<int>();
		id_stock.push_back(file_id);

		for (auto tmp = node.begin(); tmp != node.end(); ++tmp)
		{
			auto key = tmp->first;
			auto value = tmp->second;
			
			CountChildNodeId(value, id_stock);
		}
	}
}


int main(int argv, char** argc)
{
	std::string step_path = ".\\StepData\\BSP35B20-N-12.stp";
	std::string yaml_path = ".\\YamlData\\BSP35B20-N-12.yaml";

	if (argv >= 3)
	{
		step_path = argc[1];
		step_path = argc[2];
	}

	// The registry contains information about types present in the current schema; SchemaInit is a function in the schema-specific SDAI library
	Registry* registry = new Registry(SchemaInit);

	// The InstMgr holds instances that have been created or that have been loaded from a file
	InstMgr* instance_list = new InstMgr();

	// Increment FileId so entities start at #1 instead of #0.
	instance_list->NextFileId();

	// STEPfile takes care of reading and writing Part 21 files
	STEPfile* sfile = new STEPfile(*registry, *instance_list, step_path.c_str(), false);


	std::ifstream ifs(yaml_path, std::ios::in);
	if(!ifs) 
	{
		std::cerr << "YAML Parser error" << std::endl;
		return 3;
	}

	auto yaml_map = YAML::Load(ifs);
	ifs.close();

	std::vector<int> all_stock_id;

	std::vector<int> all_registered_id;
	for(int i = 0 ; i < instance_list->InstanceCount(); i ++ ) 
	{
		auto instance = instance_list->GetSTEPentity(i);

		all_registered_id.push_back(instance->GetFileId());
	}


	auto step_root = yaml_map["step"];

	for(auto tmp = step_root.begin() ; tmp != step_root.end(); tmp ++ ) 
	{
		int file_id = (*tmp)["sc_fileid"].as<int>();
		std::string file_schema = (*tmp)["sc_function"].as<std::string>();

		auto ptr = instance_list->FindFileId(file_id);
		if(ptr == nullptr) 
		{
			continue;
		}

		if(file_schema != "Shape_Definition_Representation" && 
			 file_schema != "Shape_Representation_Relationship" &&
			 file_schema != "Product_Definition_Formation_With_Specified_Source") 
		{
			continue;
		}
		
		// read valid node recursively
		YAML::Node tmp_node = *tmp;
		std::vector<int> stock_id;
		CountChildNodeId( tmp_node, stock_id);

		// Remove duplicated id
		std::copy_if(stock_id.begin(), stock_id.end(), std::back_inserter(all_stock_id),
			[&all_stock_id](const int& i) {
				return std::find(all_stock_id.begin(), all_stock_id.end(), i) == all_stock_id.end();
			});
	}

	// Extract unfiltered id
	std::vector<int> unregistered_id;
	std::copy(all_registered_id.begin(), all_registered_id.end(), std::back_inserter(unregistered_id));
	auto filtered_target = std::remove_if(unregistered_id.begin(), unregistered_id.end(),
		[&all_stock_id](const int& i) {
			return std::find(all_stock_id.begin(), all_stock_id.end(), i) != all_stock_id.end();
		});
	unregistered_id.erase(filtered_target, unregistered_id.end());

	// Remove from instmgr
	for(auto iter = unregistered_id.begin(); iter != unregistered_id.end(); ++iter ) 
	{
		auto node = instance_list->FindFileId(*iter);
		instance_list->Delete(node);
	}
	sfile->WriteExchangeFile("test.step");


	return 0;
}
