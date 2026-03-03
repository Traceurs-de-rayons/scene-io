#include "tdr/SceneSchema.hpp"
#include <iostream>
#include <math.h>

namespace sceneIO::tdr {

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

void TagSchema::include(const std::map<std::string, TagSchema>& group)
{
	for (const auto& [name, tag] : group)
		children[name] = tag;
}

void ConditionalVariant::include(const std::map<std::string, TagSchema>& group)
{
	for (const auto& [name, tag] : group)
		children[name] = tag;
}

const ConditionalVariant* TagSchema::getMatchingVariant(const std::string& discriminator_value) const
{
	for (const auto& variant : variants)
	{
		if (variant.discriminator_value == discriminator_value)
			return &variant;
	}
	return nullptr;
}

const TagSchema *SceneSchema::findTagRecursive(const TagSchema& tag, const std::string& tagName) const
{
	if (tag.name == tagName)
		return &tag;
	else
	{
		for (auto it = tag.children.begin(); it != tag.children.end(); it++)
		{
			auto res = findTagRecursive(it->second, tagName);
			if (res) return res;
		}
	}
	return nullptr;
}

const TagSchema *SceneSchema::getTagSchema(const std::string& tagName) const
{
	return findTagRecursive(root, tagName);
}

const AttributeSchema *SceneSchema::getAttributeSchema(const std::string& tagName, const std::string& attrName) const
{
	const TagSchema *tag = getTagSchema(tagName);

	if (!tag) return nullptr;

	for (auto it = tag->attributes.begin(); it != tag->attributes.end(); it++)
		if (it->first == attrName) return &it->second;

	return nullptr;
}

}
