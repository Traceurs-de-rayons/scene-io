#pragma once

#include "tdr/lexer.hpp"
#include "tdr/error.hpp"

#include <map>

namespace sceneIO::tdr {


Node parser(std::vector<Token>& list, ErrorCollector& errors);

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

};



}