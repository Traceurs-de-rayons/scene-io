#pragma once

#include <string>
#include <vector>
#include <fstream>

namespace sceneIO::tdr {

enum TokenType {
	TokenOpen,
	TokenClose,
	TokenText
};

struct Token
{
	TokenType type = TokenText;
	std::string value;
	int line;
	int column;
};

std::vector<Token> lexer(std::ifstream& in);

}
