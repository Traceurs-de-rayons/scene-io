#include "tdr/semanticAnalyzer.hpp"
#include "tdr/SceneSchema.hpp"

#include "core-utils.hpp"

#include <charconv>
#include <filesystem>
#include <algorithm>

namespace sceneIO::tdr {

static bool isValidByte(const std::string& s)
{
	if (s.empty()) return false;

	int value;
	auto [ptr, ec] = std::from_chars(
		s.data(),
		s.data() + s.size(),
		value
	);

	return ec == std::errc() && ptr == s.data() + s.size() && value >= 0 && value <= 255;
}

bool isValidColor(const std::string& s)
{
	if (s.empty()) return false;

	if (s.size() == 7 && s[0] == '#')
	{
		for (size_t i = 1; i < 7; ++i)
			if (!std::isxdigit(static_cast<unsigned char>(s[i]))) return false;
		return true;
	}

	size_t first = s.find(',');
	if (first == std::string::npos) return false;

	size_t second = s.find(',', first + 1);
	if (second == std::string::npos) return false;

	if (s.find(',', second + 1) != std::string::npos) return false;

	std::string r = s.substr(0, first);
	std::string g = s.substr(first + 1, second - first - 1);
	std::string b = s.substr(second + 1);

	return isValidByte(r) && isValidByte(g) && isValidByte(b);
}

std::string isValidFilePath(const std::string& pathStr)
{
	if (pathStr.empty()) return "Invalid file path: path is empty";

	namespace fs = std::filesystem;
	std::error_code ec;

	fs::path path(pathStr);

	if (!fs::exists(path, ec))
	{
		if (ec) return "Invalid file path: " + ec.message();
		return "Invalid file path: no such file or directory";
	}

	if (!fs::is_regular_file(path, ec))
	{
		if (ec) return "Invalid file path: " + ec.message();
		return "Invalid file path: not a regular file";
	}

	fs::perms p = fs::status(path, ec).permissions();
	if (ec) return "Invalid file path: " + ec.message();

	bool readable = (p & fs::perms::owner_read) != fs::perms::none
				 || (p & fs::perms::group_read) != fs::perms::none
				 || (p & fs::perms::others_read) != fs::perms::none;

	if (!readable) return "Invalid file path: permission denied";

	return "";
}

const std::string validType(ValueType type, const std::optional<std::pair<float, float>>& attrRange, const std::vector<std::string>& attrEnum, const std::string& param, ErrorCollector& errors)
{
	switch (type)
	{
		case ValueType::STRING: return "";
		case ValueType::FLOAT:
		{
			float val = 0;

			if (!isValidValue(param, val)) break;
			if (attrRange.has_value())
			{
				if (val > attrRange.value().second || val < attrRange.value().first)
					return "Invalid parameter '" + param + "'. Value must be between " + std::to_string(attrRange.value().first) + " and " + std::to_string(attrRange.value().second);
			}
			return "";
		}
		case ValueType::INT:
		{
			int val = 0;
			if (!isValidValue(param, val)) break;
			if (attrRange.has_value())
			{
				if (val > attrRange.value().second || val < attrRange.value().first)
					return "Invalid parameter '" + param + "'. Value must be between " + std::to_string(attrRange.value().first) + " and " + std::to_string(attrRange.value().second);
			}
			return "";
		}
		case ValueType::BOOL:
		{
			if (param == "1" || param == "0" || param == "true" || param == "false") return "";
			break;
		}
		case ValueType::VEC3:
		{
			auto parts = cu::string::split(param, ' ');

			float val0 = 0;
			float val1 = 0;
			float val2 = 0;

			if (parts.size() != 3) return "Invalid parameter '" + param + "'. Wrong amount of numbers for a vec3.";
			if (!isValidValue(parts[0], val0)) return "Invalid parameter '" + param + "'. '" + parts[0] + "' is not a valid number.";
			if (!isValidValue(parts[1], val1)) return "Invalid parameter '" + param + "'. '" + parts[1] + "' is not a valid number.";
			if (!isValidValue(parts[2], val2)) return "Invalid parameter '" + param + "'. '" + parts[2] + "' is not a valid number.";
			
			if (attrRange.has_value())
			{
				if (val0 > attrRange.value().second || val0 < attrRange.value().first)
					return "Invalid parameter '" + param + "'. '" + parts[0] + "' is not a valid number. Value must be between " + std::to_string(attrRange.value().first) + " and " + std::to_string(attrRange.value().second);
				if (val1 > attrRange.value().second || val1 < attrRange.value().first)
					return "Invalid parameter '" + param + "'. '" + parts[1] + "' is not a valid number. Value must be between " + std::to_string(attrRange.value().first) + " and " + std::to_string(attrRange.value().second);
				if (val2 > attrRange.value().second || val2 < attrRange.value().first)
					return "Invalid parameter '" + param + "'. '" + parts[2] + "' is not a valid number. Value must be between " + std::to_string(attrRange.value().first) + " and " + std::to_string(attrRange.value().second);
			}
			return "";
		}
		case ValueType::COLOR:
		{
			if (isValidColor(param)) return "";
			break;
		}
		case ValueType::FILEPATH:
		{
			return isValidFilePath(param);
		}
		case ValueType::ENUM:
		{
			if (std::find(attrEnum.begin(), attrEnum.end(), param) != attrEnum.end()) return "";
			std::string res = "Invalid parameter '" + param + "'. Parameter must be one of [";
			for (const std::string& option : attrEnum)
				res += "'" + option + "', ";
			res[res.size() - 2] = ']';
			res[res.size() - 1] = '.';
			return res;
		}
		default: break;
	}
	return "Invalid parameter type. Required: " + printValueType(type);
}

void analyseAttributes(const Node& tag, const TagSchema& tagSchema, ErrorCollector& errors)
{
	auto& attrs = tag.getAttributes();
	auto& allowedAttrs = tagSchema.attributes;

	for (auto attr = attrs.begin(); attr != attrs.end(); attr++)
	{
		auto attrSchema = allowedAttrs.find(attr->first);
		if (attrSchema == allowedAttrs.end())
		{
			errors.report(TdrError(attr->second.attr_line, attr->second.attr_column, 2, "Unknown property '" + attr->first + "'"));
			continue ;
		}

		const std::string typeError = validType(attrSchema->second.type, attrSchema->second.range, attrSchema->second.enum_values, attr->second.content, errors);
		if (!typeError.empty()) errors.report(TdrError(attr->second.content_line, attr->second.content_column, typeError));
	}

	for (auto requiredAttr = allowedAttrs.begin(); requiredAttr != allowedAttrs.end(); requiredAttr++)
	{
		if (!requiredAttr->second.required) continue ;
		if (attrs.find(requiredAttr->first) == attrs.end())
		{
			auto pos = tag.getNodeBeginPos();
			errors.report(TdrError(pos.first, pos.second, "Missing required property '" + requiredAttr->first + "'"));
		}
	}
}

/*

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

*/

void validateMultiplicity(const Node& parent, const TagSchema& parentSchema, ErrorCollector& errors)
{
	std::map<std::string, int> tagCounts;
	
	for (const auto& child : parent.getChildren())
		tagCounts[child.getIdentifier()]++;
	
	for (const auto& [tagName, count] : tagCounts)
	{
		auto tagSchemaPair = parentSchema.children.find(tagName);
		if (tagSchemaPair != parentSchema.children.end())
		{
			if (!tagSchemaPair->second.allow_multiple && count > 1)
			{
				auto pos = parent.getNodeBeginPos();
				errors.report(TdrError(pos.first, pos.second, 1, "Tag '" + tagName + "' appears " + std::to_string(count) + " times but is not allowed to repeat"));
			}
		}
	}
}

TagSchema buildEffectiveSchema(const TagSchema& base, const Node& node)
{
	if (base.variants.empty())
		return base;

	auto& attrs = node.getAttributes();

	for (const auto& variant : base.variants)
	{
		auto it = attrs.find(variant.discriminator_attr);
		if (it != attrs.end() && it->second.content == variant.discriminator_value)
		{
			TagSchema effective = base;

 			for (const auto& [name, attr] : variant.attributes)
				effective.attributes[name] = attr;

			for (const auto& [name, child] : variant.children)
				effective.children[name] = child;

			if (variant.allow_text)
			{
				effective.allow_text = variant.allow_text;
				effective.text_type = variant.text_type;
			}

			effective.variants.clear();
			effective.fromCondition = std::make_pair(variant.discriminator_attr, variant.discriminator_value);

			return effective;
		}
	}
	return base;
}

void analyzeNodes(const Node& parent, const TagSchema& parentSchema, ErrorCollector& errors)
{
	for (auto& node : parent.getChildren())
	{
		auto tagSchemaPair = parentSchema.children.find(node.getIdentifier());
		if (tagSchemaPair == parentSchema.children.end())
		{
			auto pos = node.getNodeBeginPos();
			errors.report(TdrError(pos.first, pos.second, 1, "Unknown identifier '" + node.getIdentifier() + "'"));
			continue ;
		}

		TagSchema effectiveSchema = buildEffectiveSchema(tagSchemaPair->second, node);

		auto pos = node.getTextBeginPos();
		if (!effectiveSchema.allow_text && !node.getText().empty())
		{
			errors.report(TdrError(pos.first, pos.second, 1, "Text is not allowed in '" + node.getIdentifier() + "'"));
		}
		else if (effectiveSchema.text_type.has_value())
		{
			const std::string typeError = validType(effectiveSchema.text_type.value(), effectiveSchema.range, effectiveSchema.enum_values, node.getText(), errors);
			if (!typeError.empty()) errors.report(TdrError(pos.first, pos.second, typeError));
		}

		analyseAttributes(node, effectiveSchema, errors);
		analyzeNodes(node, effectiveSchema, errors);
	}

	validateMultiplicity(parent, parentSchema, errors);

	TagSchema effectiveParentSchema = buildEffectiveSchema(parentSchema, parent);

	for (auto requiredTag = effectiveParentSchema.children.begin(); requiredTag != effectiveParentSchema.children.end(); requiredTag++)
	{
		if (!requiredTag->second.required) continue ;
		auto childExists = std::any_of(
			parent.getChildren().begin(), 
			parent.getChildren().end(),
			[&requiredTag](const Node& child) { return child.getIdentifier() == requiredTag->first; }
		);
		if (!childExists)
		{
			auto pos = parent.getNodeBeginPos();
			errors.report(TdrError(pos.first, pos.second, "Missing required tag '" + requiredTag->first + "'"));
		}
	}

	// // Also check required tags/attrs from matching variants of each child
	// for (auto& node : parent.getChildren())
	// {
	// 	auto tagSchemaPair = parentSchema.children.find(node.getIdentifier());
	// 	if (tagSchemaPair == parentSchema.children.end())
	// 		continue;

	// 	TagSchema effectiveSchema = buildEffectiveSchema(tagSchemaPair->second, node);

	// 	// Check required children from variant
	// 	for (const auto& [childName, childSchema] : effectiveSchema.children)
	// 	{
	// 		if (!childSchema.required) continue;
	// 		// Skip if already in base (handled above)
	// 		if (tagSchemaPair->second.children.count(childName))
	// 			continue;

	// 		const std::string& cn = childName;
	// 		auto childExists = std::any_of(
	// 			node.getChildren().begin(),
	// 			node.getChildren().end(),
	// 			[&cn](const Node& child) { return child.getIdentifier() == cn; }
	// 		);
	// 		if (!childExists)
	// 		{
	// 			auto pos = node.getNodeBeginPos();
	// 			errors.report(TdrError(pos.first, pos.second, "Missing required tag '" + cn + "' (required for " + tagSchemaPair->second.name + " variant)"));
	// 		}
	// 	}

	// 	// Check required attributes from variant
	// 	for (const auto& [attrName, attrSchema] : effectiveSchema.attributes)
	// 	{
	// 		if (!attrSchema.required) continue;
	// 		if (tagSchemaPair->second.attributes.count(attrName))
	// 			continue;

	// 		const std::string& an = attrName;
	// 		auto& attrs = node.getAttributes();
	// 		if (attrs.find(an) == attrs.end())
	// 		{
	// 			auto pos = node.getNodeBeginPos();
	// 			errors.report(TdrError(pos.first, pos.second, "Missing required property '" + an + "' (required for " + tagSchemaPair->second.name + " variant)"));
	// 		}
	// 	}
	// }
}

void semanticAnalyzer(Node& ast, SceneSchema& sceneSchema, ErrorCollector& errors)
{
	analyzeNodes(ast, sceneSchema.root, errors);
}

}