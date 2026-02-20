#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "tdr/error.hpp"

namespace sceneIO::tdr {

enum class TokenType {
	TAG_OPEN,		// <
	TAG_END_OPEN,	// </
	TAG_CLOSE,		// >
	TAG_SELF_CLOSE,	// />
	IDENTIFIER,		// key
	EQUALS,			// =
	STRING,			// "value"
	TEXT,			// balise content
	END_OF_FILE
};

struct Token
{
	TokenType type = TokenType::TEXT;
	std::string value;
	uint64_t line;
	uint64_t column;
};

std::vector<Token> lexer(std::istream& in, ErrorCollector& errors);
void print_tokens(const std::vector<Token>& tokens);

const std::string getTokenContent(Token& tok);

inline std::ostream& operator<<(std::ostream& io, TokenType token)
{
	switch (token)
	{
		case TokenType::TAG_OPEN:		io << "TAG_OPEN";		break;
		case TokenType::TAG_END_OPEN:	io << "TAG_END_OPEN";	break;
		case TokenType::TAG_CLOSE:		io << "TAG_CLOSE";		break;
		case TokenType::TAG_SELF_CLOSE:	io << "TAG_SELF_CLOSE";	break;
		case TokenType::IDENTIFIER:		io << "IDENTIFIER";		break;
		case TokenType::EQUALS:			io << "EQUALS";			break;
		case TokenType::STRING:			io << "STRING";			break;
		case TokenType::TEXT:			io << "TEXT";			break;	
		default:
			break;
	}
	return io;
}

}
