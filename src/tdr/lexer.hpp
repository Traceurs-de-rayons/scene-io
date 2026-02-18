#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace sceneIO::tdr {

enum class TokenType {
	TAG_OPEN,		// <
	TAG_END_OPEN,	// </
	TAG_CLOSE,		// >
	TAG_SELF_CLOSE,	// />
	IDENTIFIER,		// key
	EQUALS,			// =
	STRING,			// "value"
	TEXT			// balise content
};

struct Token
{
	TokenType type = TokenType::TEXT;
	std::string value;
	int line;
	int column;
};

std::vector<Token> lexer(std::ifstream& in);
void print_tokens(const std::vector<Token>& tokens);


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
