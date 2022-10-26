#include <STEPfile.h>
#include <sdai.h>
#include <STEPattribute.h>
#include <ExpDict.h>
#include <Registry.h>

#include <regex>
#include <STEPaggrEntity.h>
#include <STEPcomplex.h>

#include "schema.h"

#include <yaml-cpp/yaml.h>

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

bool ExtractSelectInstanceId(InstMgr*& inst_mgr, SDAI_Select* select, BASE_TYPE &recog_type, int& answer_id)
{

	auto type = select->ValueType();
	recog_type = type;

	if ((type & sdaiINSTANCE) == 0)
	{
		return true;
	}

	std::string out_text;
	select->STEPwrite(out_text);

	// ap203 only?
	std::regex id_detect_regex("\\#(\\d+)");
	std::smatch match;
	if (!std::regex_search(out_text, match, id_detect_regex) ||
		match.length() < 2)
	{
		std::cout << "Regex Pattern Match failed" << std::endl;
		return false;
	}

	int id = 0;
	try
	{
		id = std::stol(match[1].str());
	}
	catch (exception)
	{
		std::cout << "Regex ID error" << std::endl;
		return false;
	}

	answer_id = id;

	return true;
}

void AddNode(InstMgr*& inst_mgr, std::vector<int>& stock_id, SDAI_Application_instance* const& instance, std::stringstream& debug_log, YAML::Node& yaml_node, int loop_count, bool is_complex)
{
	int id = instance->GetFileId();

	if(!is_complex) 
	{
		yaml_node["sc_fileid"] = id;
		stock_id.push_back(id);
	}
	yaml_node["sc_function"] = instance->EntityName();


	auto instance_complex = dynamic_cast<const STEPcomplex*>(instance);
	if(instance_complex != nullptr && !is_complex)  
	{
		for (auto iter = instance_complex->head; iter != nullptr; iter = iter->sc)
		{
			auto inst = dynamic_cast<SDAI_Application_instance*>(iter);

			YAML::Node yaml_child_node;
			AddNode(inst_mgr, stock_id, inst, debug_log, yaml_child_node, loop_count + 1, true);
			yaml_node["complex_element"].push_back(yaml_child_node);
		}

		return;
	}

	for (int j = 0; j < instance->AttributeCount(); j++)
	{
		auto attribute = &instance->attributes[j];

		auto attr_select = attribute->Select();
		auto attr_instance = attribute->Entity();
		auto attr_aggr = attribute->Aggregate();

		PrintDebugMessage(id, attribute, debug_log, loop_count);

		if (!attr_select && !attr_instance && !attr_aggr)
		{
			auto name = attribute->Name();
			yaml_node[name] = attribute->asStr();
			continue;
		}

		if (attribute->IsDerived())
		{
			yaml_node[attribute->Name()] = attribute->asStr();
		}
		else if(attr_aggr != nullptr)
		{
			auto head_node = attr_aggr->GetHead();

			std::vector<std::string> sub_node_vec;
			std::vector<YAML::Node> aggr_node_vec;

			for (auto node = head_node; node != nullptr; node = node->next)
			{
				auto conv_node = dynamic_cast<const EntityNode*>(node);
				auto select_node = dynamic_cast<const SelectNode*>(node);
				auto conv_raw_node = dynamic_cast<STEPnode*>(node);

				if(conv_node != nullptr) 
				{
					auto inst = conv_node->node;

					YAML::Node yaml_child_node;
					AddNode(inst_mgr, stock_id, inst, debug_log, yaml_child_node, loop_count + 1, false);

					aggr_node_vec.push_back(yaml_child_node);
				}
				else if(select_node != nullptr) 
				{
					PrintDebugMessage(id, nullptr, debug_log, loop_count + 1);

					BASE_TYPE recog_type;
					int answer_id;
					if(!ExtractSelectInstanceId(inst_mgr, select_node->node, recog_type, answer_id)) 
					{
						continue;
					}

					if((recog_type & sdaiINSTANCE) != 0) 
					{
						auto select_entity = inst_mgr->FindFileId(answer_id);
						if (select_entity == nullptr)
						{
							std::cout << "Regex Conversion error" << std::endl;
							continue;
						}
						auto select_instance = select_entity->GetApplication_instance();

						YAML::Node yaml_child_node;
						AddNode(inst_mgr, stock_id, select_instance, debug_log, yaml_child_node, loop_count + 1, false);
						aggr_node_vec.push_back(yaml_child_node);
					}
					else 
					{
						std::string str;
						conv_raw_node->asStr(str);
						sub_node_vec.push_back(str);
					}
				}
				else if(conv_raw_node != nullptr) 
				{
					PrintDebugMessage(id, nullptr, debug_log, loop_count + 1);

					std::string str;
					conv_raw_node->asStr(str);
					sub_node_vec.push_back(str);
				}
				else 
				{
					PrintDebugMessage(id, nullptr, debug_log, loop_count + 1);
				}
			}

			if(is_complex) 
			{
				if (aggr_node_vec.size() != 0)
				{
					yaml_node[attribute->Name()]["aggregate_element"] = aggr_node_vec;
				}
				else if (sub_node_vec.size() != 0)
				{
					yaml_node[attribute->Name()]["aggregate_element"] = sub_node_vec;
				}
			}
			else if(aggr_node_vec.size() != 0 )
			{
				yaml_node[attribute->Name()] = aggr_node_vec;
			}
			else if(sub_node_vec.size() != 0) 
			{
				yaml_node[attribute->Name()] = sub_node_vec;
			}
		}
		else if(attr_select != nullptr) 
		{
			BASE_TYPE recog_type;
			int answer_id;
			if(!ExtractSelectInstanceId(inst_mgr, attr_select, recog_type, answer_id)) 
			{
				continue;
			}

			if((recog_type & sdaiINSTANCE ) == 0) 
			{
				continue;
			} 
			
			auto select_entity = inst_mgr->FindFileId(answer_id);
			if (select_entity == nullptr)
			{
				std::cout << "Regex Conversion error" << std::endl;
				continue;
			}
			auto select_instance = select_entity->GetApplication_instance();

			YAML::Node yaml_child_node;
			AddNode(inst_mgr, stock_id, select_instance, debug_log, yaml_child_node, loop_count + 1, false);

			yaml_node[attribute->Name()] = yaml_child_node;
		}
		else if(attr_instance != nullptr)
		{
			YAML::Node yaml_child_node;
			AddNode(inst_mgr, stock_id, attr_instance, debug_log, yaml_child_node, loop_count + 1, false);

			yaml_node[attribute->Name()] = yaml_child_node;
		}
	}
}


int main( int argv, char** argc)
{
	std::string path = ".\\StepData\\simple_shapes.stp";
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
	STEPfile* sfile = new STEPfile(*registry, *instance_list, path.c_str(), false);

	std::stringstream debug_log;
	YAML::Node root_node;

	std::vector<int> stock_id;

	for(int i = 0 ; i < instance_list->InstanceCount(); i++ ) 
	{
		auto instance = instance_list->GetSTEPentity(i);
		int file_id = instance->GetFileId();

		if(std::find(stock_id.begin(), stock_id.end(), file_id) == stock_id.end())
		{
			std::cout << "#" << file_id << "export" << std::endl;
			debug_log << file_id << ":root(" << instance->EntityName() << ")" << std::endl;

			YAML::Node step_node;
			AddNode(instance_list, stock_id, instance, debug_log, step_node, 1, false);

			std::stringstream ss_str;
			ss_str << "#" << instance->GetFileId();
			root_node.push_back(step_node);
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
	YAML::Node master_node;
	master_node["step"] = root_node;
	yaml_emitter << master_node;
	ofs_yaml.close();

	return 0;
}
