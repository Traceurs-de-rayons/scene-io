#include "tdr/lexer.hpp"
#include "tdr/parser.hpp"
#include "tdr/error.hpp"
#include "tdr/SceneSchema.hpp"
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
	static std::string get_hover(const Node& ast, const SceneSchema& schema, size_t line, size_t col);

private:
	static std::string find_hover_recursive(const Node& node, const TagSchema& schema, size_t line, size_t col);
};

}
