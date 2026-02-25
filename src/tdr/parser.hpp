#pragma once

#include "tdr/lexer.hpp"
#include "tdr/error.hpp"

#include <map>

namespace sceneIO::tdr {

class SceneSchema;

struct AttributeInfos
{
	std::string content;
	uint64_t attr_line = UINT64_MAX;
	uint64_t attr_column = UINT64_MAX;
	uint64_t content_line = UINT64_MAX;
	uint64_t content_column = UINT64_MAX;
};

class Node
{
private:
	std::string identifier_;
	std::vector<Node> children_;
	std::map<std::string, AttributeInfos> attributes_;
	std::string text_;

	std::vector<Token> tokens_;

	friend Node parser(std::vector<Token>& list, ErrorCollector& errors);
	friend void semanticAnalyzer(Node& ast, SceneSchema& sceneSchema, ErrorCollector& errors);

public:
	Node(const std::string& identifier = "root") : identifier_(identifier) {}
	~Node() {}

	const std::string& getIdentifier() const { return identifier_; }
	const std::string& getText() const { return text_; }
	const std::vector<Node>& getChildren() const { return children_; }
	const std::map<std::string, AttributeInfos>& getAttributes() const { return attributes_; }
	const std::vector<Token>& getTokens() const { return tokens_; }

	const std::pair<uint64_t, uint64_t> getNodeBeginPos() const;
	const std::pair<uint64_t, uint64_t> getTextBeginPos() const;

	void print(int nest = 0) const;

};

Node parser(std::vector<Token>& list, ErrorCollector& errors);

}