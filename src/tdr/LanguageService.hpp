#include "tdr/lexer.hpp"
#include "tdr/parser.hpp"
#include "tdr/error.hpp"
#include <sstream>

namespace sceneIO::tdr {

struct ParseResult {
	Node ast;
	std::vector<TdrError> errors;
};

class SceneLanguageService
{
public:
	static ParseResult parse_file(const std::string& filepath);
	static ParseResult parse_content(const std::string& content);
	
	// std::vector<CompletionItem> get_completions(const std::string& content, int line, int col); // my dream
	// std::optional<HoverInfo> get_hover(const std::string& content, int line, int col); // my dream v2
};

}
