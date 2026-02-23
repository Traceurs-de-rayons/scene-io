#pragma once

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

const std::string validType(const AttributeSchema& attr, const std::string& param, ErrorCollector& errors)
{
	ValueType type = attr.type;

	switch (type)
	{
		case ValueType::STRING: return "";
		case ValueType::FLOAT:
		{
			if (!isValidValue<float>(param)) break;
			if (attr.range.has_value())
			{
				float val = std::stof(param);
				
				if (val > attr.range.value().second || val < attr.range.value().first)
					return "Invalid parameter '" + param + "'. Value must be between " + std::to_string(attr.range.value().first) + " and " + std::to_string(attr.range.value().second);
			}
			return "";
		}
		case ValueType::INT:
		{
			if (!isValidValue<int>(param)) break;
			if (attr.range.has_value())
			{
				int val = std::stoi(param);
				
				if (val > attr.range.value().second || val < attr.range.value().first)
					return "Invalid parameter '" + param + "'. Value must be between " + std::to_string(attr.range.value().first) + " and " + std::to_string(attr.range.value().second);
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
			if (parts.size() != 3) return "Invalid parameter '" + param + "'. Wrong amount of numbers for a vec3";
			if (!isValidValue<float>(parts[0])) return "Invalid parameter '" + param + "'. Wrong amount of numbers for a vec3";
			
			//&& isValidValue<float>(parts[0]) && isValidValue<float>(parts[1]) && isValidValue<float>(parts[2])) return "";
			break;
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
			if (std::find(attr.enum_values.begin(), attr.enum_values.end(), param) != attr.enum_values.end()) return "";
			std::string res = "Invalid parameter '" + param + "'. Parameter must be one of [";
			for (const std::string& option : attr.enum_values)
				res += "'" + option + "', ";
			res[res.size() - 2] = ']';
			res[res.size() - 1] = '.';
			return res;
		}
		default: break;
	}
	return "Invalid parameter type. Required: " + printValueType(type);
}

/*

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

*/

void analyseAttributes(Node& tag, TagSchema& tagSchema, ErrorCollector& errors)
{
	auto& attrs = tag.getAttributes();
	auto& allowedAttrs = tagSchema.attributes;

	for (auto attr = attrs.begin(); attr != attrs.end(); attr++)
	{
		if (allowedAttrs.find(attr->first) == allowedAttrs.end())
		{
			errors.report(TdrError(attr->second.attr_line, attr->second.attr_column, 2, "Unknown property '" + attr->first + "'"));
			continue ;
		}

		
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

void semanticAnalyzer(Node& ast, ErrorCollector& errors)
{

}

}