#include "tdr/lexer.hpp"
#include "tdr/error.hpp"

#include <algorithm> 
#include <cctype>
#include <locale>

namespace sceneIO::tdr {

void ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

void rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

void trim(std::string &s)
{
	rtrim(s);
	ltrim(s);
}

std::vector<Token> lexer(std::ifstream& in)
{
	std::vector<Token> tokens;

	std::string line;
	uint64_t line_count = 0;

	Token currentToken;

	while (getline(in, line))
	{
		uint64_t col = 0;
		line_count++;

		for (char c : line)
		{
			col++;

			// if (std::isspace(c) && (currentToken.type == TokenOpen || currentToken.type == TokenClose))
			// {
			// 	throw TdrError({.line = line_count, .column = col}, "Unexpected 'char');
			// }
			if (c == '<')
			{
				if (currentToken.type == TokenOpen) throw TdrError({.line = line_count, .column = col}, "Expected '>' after '<" + currentToken.value + "'");
				else if (currentToken.type == TokenClose) throw TdrError({.line = line_count, .column = col}, "Expected '>' after '</" + currentToken.value + "'");
				else
				{
					trim(currentToken.value);
					if (!currentToken.value.empty()) tokens.push_back(currentToken);

					// changer le state du token
				}
			}
		}
	}

	return tokens;
}

}