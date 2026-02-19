#pragma once

#include "tdr/lexer.hpp"
#include "tdr/error.hpp"

#include <map>

namespace sceneIO::tdr {

class ErrorCollector
{

private:
	std::vector<TdrError> errors_;
	
public:
	ErrorCollector() = default;
	~ErrorCollector() = default;

	void report(TdrError error) { errors_.push_back(std::move(error)); }
	
	bool has_errors() const { return !errors_.empty(); }
	const std::vector<TdrError>& get_errors() const { return errors_; }
};

class Node
{
private:
	std::string identifier_;
	std::vector<Node> children_;
	std::map<std::string, std::string> attributes_;
	std::string text_;

	std::vector<Token> tokens_;

	friend Node parser(std::vector<Token>& list, ErrorCollector& errors);

public:
	Node(const std::string& identifier = "root") : identifier_(identifier) {}
	~Node() {}

	const std::string& getIdentifier() const { return identifier_; }
	const std::string& getText() const { return text_; }
	const std::vector<Node>& getChildren() const { return children_; }
	const std::map<std::string, std::string>& getAttributes() const { return attributes_; }

	void print(int nest = 0) const;

};

Node parser(std::vector<Token>& list, ErrorCollector& errors);

}