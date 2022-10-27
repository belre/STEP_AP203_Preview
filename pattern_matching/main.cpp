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


struct DepthCount
{
public:
	int _id;
	int _depth;

public:
	DepthCount(int id, int depth) : _id(id), _depth(depth)
	{
		
	}
};

void CountChildNodeId(int depth, YAML::Node& node, std::vector<DepthCount> &depth_stock)
{
	if(node.IsSequence())
	{
		for (auto tmp = node.begin(); tmp != node.end(); ++tmp)
		{
			CountChildNodeId(depth, *tmp, depth_stock);
		}
	}
	else if(node.IsMap()) 
	{
		if (node["sc_fileid"])
		{
			int file_id = node["sc_fileid"].as<int>();
			depth_stock.emplace_back(DepthCount(file_id, depth));

			depth++;
		}
		
		for (auto tmp = node.begin(); tmp != node.end(); ++tmp)
		{
			auto key = tmp->first;
			auto value = tmp->second;
			
			CountChildNodeId(depth, value, depth_stock);
		}
	}
}

void ExtractUnfilteredId(InstMgr* instance_list, YAML::Node yaml_map, std::vector<int>& unregistered_id, std::vector<DepthCount> &all_depth_count)
{
	unregistered_id.clear();
	auto step_root = yaml_map["step"];

	std::vector<int> all_stock_id;
	std::vector<int> all_registered_id;

	for (auto tmp = step_root.begin(); tmp != step_root.end(); tmp++)
	{
		int file_id = (*tmp)["sc_fileid"].as<int>();
		std::string file_schema = (*tmp)["sc_function"].as<std::string>();

		auto ptr = instance_list->FindFileId(file_id);
		if (ptr == nullptr)
		{
			continue;
		}

		// read valid node recursively
		YAML::Node tmp_node = *tmp;
		std::vector<int> stock_id;
		std::vector<DepthCount> depth_count;
		CountChildNodeId(0, tmp_node,  depth_count);

		for(auto iter = depth_count.begin(); iter != depth_count.end(); iter++) 
		{
			stock_id.push_back(iter->_id);
		}

		
		// Remove duplicated id
		std::copy_if(stock_id.begin(), stock_id.end(), std::back_inserter(all_stock_id),
			[&all_stock_id](const int& i) {
				return std::find(all_stock_id.begin(), all_stock_id.end(), i) == all_stock_id.end();
			});

		std::copy(depth_count.begin(), depth_count.end(), std::back_inserter(all_depth_count));
	}
	std::sort(all_stock_id.begin(), all_stock_id.end());

	// Generate All Registered Id
	for (int i = 0; i < instance_list->InstanceCount(); i++)
	{
		auto instance = instance_list->GetSTEPentity(i);

		all_registered_id.push_back(instance->GetFileId());
	}

	// Extract unfiltered id
	std::copy(all_registered_id.begin(), all_registered_id.end(), std::back_inserter(unregistered_id));
	auto filtered_target = std::remove_if(unregistered_id.begin(), unregistered_id.end(),
		[&all_stock_id](const int& i) {
			return std::find(all_stock_id.begin(), all_stock_id.end(), i) != all_stock_id.end();
		});
	unregistered_id.erase(filtered_target, unregistered_id.end());

}



int main(int argv, char** argc)
{
	std::string step_path = ".\\StepData\\simple_shapes.stp";
	std::string yaml_path = ".\\YamlData\\BSP35B20-N-12-gear.yaml";

	int filter_mode = 0;

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


	if(!filter_mode) 
	{
		return 0;
	}

	std::ifstream ifs(yaml_path, std::ios::in);
	if(!ifs) 
	{
		std::cerr << "YAML Parser error" << std::endl;
		return 3;
	}

	auto yaml_map = YAML::Load(ifs);
	ifs.close();

	// Project STEP File by YAML
	std::vector<int> unfiltered_id;
	std::vector<DepthCount> depth_count;
	ExtractUnfilteredId(instance_list, yaml_map, unfiltered_id, depth_count);


	// generate including indent file
	std::stringstream ss_indent;
	std::stringstream ss_non_indent;
	std::stringstream ss_non_indent_duplicated;

	// Remove from instmgr
	for(auto iter = unfiltered_id.begin(); iter != unfiltered_id.end(); ++iter )
	{
		auto node = instance_list->FindFileId(*iter);
		instance_list->Delete(node);
	}
	sfile->WriteExchangeFile("project.step");


	for (auto iter = depth_count.begin(); iter != depth_count.end(); ++iter)
	{
		bool is_duplicated_id = false;
		if (iter != depth_count.begin())
		{
			is_duplicated_id = std::find_if(depth_count.begin(), iter - 1, [iter](const DepthCount& count) {
				return count._id == iter->_id;
				}) != iter - 1;
		}

		for (int j = 0; j < iter->_depth; j++)
		{
			if(!is_duplicated_id) 
			{
				ss_indent << "  ";
			}

			ss_non_indent_duplicated << "  ";
		}

		auto node = instance_list->FindFileId(iter->_id);
		if (node == nullptr)
		{
			ss_indent << "(" << iter->_id << "):Node Not Detected" << std::endl;
			continue;
		}

		auto inst = node->GetApplication_instance();

		std::string text;


		inst->STEPwrite(ss_non_indent_duplicated);

		if(!is_duplicated_id) 
		{
			inst->STEPwrite(ss_indent);
			inst->STEPwrite(ss_non_indent);
		}
	}

	std::ofstream ofs_indent("indent.step.txt");
	if (!ofs_indent)
	{
		std::cerr << "indent.step.txt create error" << std::endl;
		return 3;
	}
	ofs_indent << ss_indent.str();
	ofs_indent.close();

	std::ofstream ofs_nonindent("non_indent.step.txt");
	if (!ofs_nonindent)
	{
		std::cerr << "non_indent.step.txt create error" << std::endl;
		return 3;
	}
	ofs_nonindent << ss_non_indent.str();
	ofs_nonindent.close();

	std::ofstream ofs_nonindent_duplicate("non_indent_duplicated.step.txt");
	if (!ofs_nonindent_duplicate)
	{
		std::cerr << "non_indent_duplicated.step.txt create error" << std::endl;
		return 3;
	}
	ofs_nonindent_duplicate << ss_non_indent_duplicated.str();
	ofs_nonindent_duplicate.close();


	return 0;
}
