#include "tdr/LanguageService.hpp"
#include "tdr/SceneSchema.hpp"
#include "tdr/semanticAnalyzer.hpp"
#include "logger.hpp"

#include <iomanip>

namespace sceneIO::tdr {

ParseResult SceneLanguageService::parse_content(const std::string& content)
{
	ErrorCollector errors;

	try
	{
		std::istringstream stream(content);

		auto tokens = lexer(stream, errors);
		Node ast = parser(tokens, errors);

		SceneSchema sch;
		semanticAnalyzer(ast, sch, errors);

		return {std::move(ast), errors.get_errors()};
	}
	catch(const TdrError& e)
	{		
		errors.report(e);
	}
	catch (const std::exception& e)
	{
		errors.report(TdrError(e.what()));
	}
	Node empty;
	return {std::move(empty), errors.get_errors()};
}

ParseResult SceneLanguageService::parse_file(const std::string& path)
{
	ErrorCollector errors;

	try
	{
		std::ifstream in(path, std::ios_base::in);
		if (!in.is_open()) throw TdrError("Cannot open file");

		auto tokens = lexer(in, errors);
		Node ast = parser(tokens, errors);

		SceneSchema sch;
		semanticAnalyzer(ast, sch, errors);

		errors.setFilePath(path);
		return {std::move(ast), errors.get_errors()};
	}
	catch(TdrError& e)
	{
		e.location.filepath = path;
		errors.report(e);
	}
	catch (const std::exception& e)
	{
		SourceLocation loc = { .filepath = path };
		errors.report(TdrError(loc, e.what()));
	}
	Node empty;
	return {std::move(empty), errors.get_errors()};
}

std::string formatFloat(const float nb)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(3) << nb;
	return out.str();
}

std::string formatValueType(ValueType type, const std::optional<std::pair<float,float>>& range)
{
	std::string s;
	switch (type)
	{
		case ValueType::FLOAT:		s = "float";	break;
		case ValueType::INT:		s = "int";		break;
		case ValueType::STRING:		s = "string";	break;
		case ValueType::VEC3:		s = "vec3";		break;
		case ValueType::ENUM:		s = "enum";		break;
		case ValueType::BOOL:		s = "bool";		break;
		case ValueType::FILEPATH:	s = "filepath";	break;
		case ValueType::COLOR:		s = "color";	break;
		default: s = "unknown"; break;
	}
	if (range) s += " [" + formatFloat(range->first) + ", " + formatFloat(range->second) + "]";
	return s;
}

static std::string formatTagHover(const TagSchema& tag)
{
	std::ostringstream out;

	auto conditionalAttributeName = [&]() -> std::string
	{
		if (tag.fromCondition.has_value()) return tag.fromCondition.value().first;
		return "";
	};

	out << "```xml\n<" << tag.name;
	if (tag.fromCondition.has_value())
		out << " " << tag.fromCondition.value().first << "=\"" << tag.fromCondition.value().second << "\"";

	for (auto& [name, attr] : tag.attributes)
		if (attr.required && name != conditionalAttributeName())
			out << " " << name << "=\"...\"";
	for (auto& [name, attr] : tag.attributes)
		if (!attr.required && name != conditionalAttributeName())
			out << " [" << name << "=\"...\"]";
	out << ">\n```\n---\n";

	if (!tag.hover_info.empty()) out << tag.hover_info << "\n\n";

	if (tag.children.size() == 1) out << "**Child**\n\n";
	else if (tag.children.size() > 1) out << "**Children**\n\n";
	for (auto& [name, child] : tag.children)
	{
		out << "- `<" << name << ">`";
		out << (child.required ? " *(required)*" : " *(optional)*");

		if (child.allow_text && child.text_type) out << " — " << formatValueType(child.text_type.value(), child.range);

		if (!child.enum_values.empty() && child.enum_values.size() <= 4)
		{
			out << " — ";
			for (size_t i = 0; i < child.enum_values.size(); ++i)
			{
				if (i) out << " | ";
				out << "`" << child.enum_values[i] << "`";
			}
		}
		out << "\n";
	}

	if (!tag.examples.empty()) out << "*Exemple :*\n```xml\n" << tag.examples[0] << "\n```\n";

	return out.str();
}

static std::string formatAttributeHover(const AttributeSchema& attr, const std::string& parent_tag)
{
	std::ostringstream out;

	out << "```\n(attribute) " << attr.name << ": ";
	out << formatValueType(attr.type, attr.range);
	if (attr.default_value)
		out << " = " << *attr.default_value;
	out << "\n```\n---\n";

	if (!attr.hover_info.empty())
		out << attr.hover_info << "\n\n";

	if (!attr.enum_values.empty())
	{
		out << "*Values :* ";
		for (size_t i = 0; i < attr.enum_values.size(); ++i)
		{
			if (i) out << " | ";
			out << "`" << attr.enum_values[i] << "`";
		}
		out << "\n\n";
	}

	if (!attr.examples.empty())
		out << "*Exemple :* `" << attr.examples[0] << "`\n";

	return out.str();
}


std::string SceneLanguageService::find_hover_recursive(const Node& node, const TagSchema& schema, size_t line, size_t col)
{
	const auto& tokens = node.getTokens();

	for (const auto& token : tokens)
	{
		if (token.type == TokenType::IDENTIFIER && token.value == node.getIdentifier()
			&& token.line == line
			&& col >= token.column
			&& col < token.column + token.value.size())
		{
			return formatTagHover(schema);
		}
	}

	for (const auto& [attrName, attrInfo] : node.getAttributes())
	{
		if (attrInfo.attr_line == line
			&& col >= attrInfo.attr_column
			&& col < attrInfo.attr_column + attrName.size())
		{
			auto attrIt = schema.attributes.find(attrName);
			if (attrIt == schema.attributes.end()) return "";

			return formatAttributeHover(attrIt->second, node.getIdentifier());
		}

		if (attrInfo.content_line != UINT64_MAX
			&& attrInfo.content_line == line
			&& col >= attrInfo.content_column - 1
			&& col < attrInfo.content_column + attrInfo.content.size() + 1)
		{
			auto attrIt = schema.attributes.find(attrName);
			if (attrIt == schema.attributes.end()) return "";

			return formatAttributeHover(attrIt->second, node.getIdentifier());
		}
	}

	for (const auto& child : node.getChildren())
	{
		auto tagSchema = schema.children.find(child.getIdentifier());

		if (tagSchema == schema.children.end()) continue;

		auto sch = buildEffectiveSchema(tagSchema->second, child);

		std::string result = find_hover_recursive(child, sch, line, col);
		if (!result.empty())
			return result;
	}

	return "";
}

std::string SceneLanguageService::get_hover(const Node& ast, const SceneSchema& schema, size_t line, size_t col)
{
	for (const auto& child : ast.getChildren())
	{
		auto tagSchema = schema.root.children.find(child.getIdentifier());

		if (tagSchema == schema.root.children.end()) continue;

		auto sch = buildEffectiveSchema(tagSchema->second, child);

		std::string result = find_hover_recursive(child, sch, line, col);
		if (!result.empty())
			return result;
	}
	return "";
}

}
