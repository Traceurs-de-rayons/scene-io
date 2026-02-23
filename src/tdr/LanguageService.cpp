#include "tdr/LanguageService.hpp"
#include "logger.hpp"

namespace sceneIO::tdr {

ParseResult SceneLanguageService::parse_content(const std::string& content)
{
	ErrorCollector errors;

	try
	{
		std::istringstream stream(content);

		auto tokens = lexer(stream, errors);
		Node ast = parser(tokens, errors);

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

const std::string get_hover(Node& ast)
{
	
}

}
