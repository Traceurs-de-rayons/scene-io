#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace sceneIO::tdr {

enum class ValueType
{
	STRING,
	FLOAT,
	INT,
	BOOL,
	VEC3,
	COLOR,
	FILEPATH,
	ENUM
};

const std::string printValueType(ValueType type);

struct AttributeSchema
{
	std::string name;
	bool required;
	ValueType type;
	std::optional<std::string> default_value;

	std::optional<std::pair<float, float>> range;	// INT/FLOAT/VEC
	std::vector<std::string> enum_values;			// ENUM

	std::string hover_info;
	std::string completion_detail;

	std::vector<std::string> examples;
};

struct TagSchema
{
	std::string name;
	bool required;
	bool allow_text;
	std::optional<ValueType> text_type;

	std::optional<std::pair<float, float>> range;	// INT/FLOAT/VEC
	std::vector<std::string> enum_values;			// ENUM

	std::map<std::string, AttributeSchema> attributes;
	std::map<std::string, TagSchema> children;
	
	std::string hover_info;
	std::string completion_detail;
	std::vector<std::string> examples;

	bool allow_multiple;
};

class SceneSchema
{

private:
	void build_schema();

	const TagSchema *findTagRecursive(const TagSchema& tag, const std::string& tag_name) const;

public:
	TagSchema root;
	
	SceneSchema() { build_schema(); }
	~SceneSchema() = default;

	const TagSchema *getTagSchema(const std::string& tag_name) const;
	const AttributeSchema *getAttributeSchema(const std::string& tag_name, const std::string& attr_name) const;

};

}