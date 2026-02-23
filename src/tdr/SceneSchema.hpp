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

const std::string printValueType(ValueType type)
{
	switch (type)
	{
		case ValueType::STRING:		return "string";
		case ValueType::FLOAT:		return "float";
		case ValueType::INT:		return "integer";
		case ValueType::BOOL:		return "boolean";
		case ValueType::VEC3:		return "vec3";
		case ValueType::COLOR:		return "color";
		case ValueType::FILEPATH:	return "filepath";
		case ValueType::ENUM:		return "enum";
		default: break;
	}
	return "";
}

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

	std::map<std::string, AttributeSchema> attributes;
	std::map<std::string, TagSchema> children;
	
	bool allow_multiple;

	std::string hover_info;
	std::string completion_detail;
	std::vector<std::string> examples;
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