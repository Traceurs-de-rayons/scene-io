#include "tdr/lexer.hpp"
#include "tdr/error.hpp"
#include "colors.hpp"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <iomanip>

namespace sceneIO::tdr {

std::vector<Token> lexer(std::ifstream& in)
{
	std::vector<Token> tokens;

	uint64_t line = 1;
	uint64_t column = 1;
	char c;
	
	auto peek = [&]() -> char
	{
		int next = in.peek();
		return (next == EOF) ? '\0' : static_cast<char>(next);
	};
	
	auto advance = [&]() -> char
	{
		if (!in.get(c)) return '\0';
		if (c == '\n')
		{
			line++;
			column = 1;
		}
		else column++;
		return c;
	};
	
	auto skip_whitespace_in_tag = [&]()
	{
		while (std::isspace(peek()) && peek() != '\0') advance();
	};
	
	auto skip_comment = [&]()
	{
		if (peek() == '#')
			while (peek() != '\n' && peek() != '\0')
				advance();
	};
	
	auto read_identifier = [&](int start_line, int start_col) -> Token
	{
		std::string value;

		while (std::isalnum(peek()) || peek() == '_' || peek() == '-')
			value += advance();

		return {TokenType::IDENTIFIER, value, start_line, start_col};
	};
	
	auto read_string = [&](int start_line, int start_col) -> Token
	{
		std::string value;
		char quote = c;
		
		while (true)
		{
			if (peek() == quote)
			{
				advance();
				break;
			}
			else if (peek() == '\0') throw TdrError(line, column, "Unterminated string literal");
			else if (peek() == '\n') throw TdrError(line, column, "Newline in string literal");

			advance();
			if (c == '\\')
			{
				if (peek() == '\0') throw TdrError(line, column, "Unterminated escape sequence");

				advance();
				switch (c)
				{
					case 'n':	value += '\n';	break;
					case 't':	value += '\t';	break;
					case '\\':	value += '\\';	break;
					case '"':	value += '"';	break;
					case '\'':	value += '\'';	break;
					default:	value += c;		break;
				}
			}
			else value += c;
		}
		
		return {TokenType::STRING, value, start_line, start_col};
	};
	
	auto read_text = [&](int start_line, int start_col) -> Token
	{
		std::string value;
		
		while (peek() != '<' && peek() != '\0')
		{
			if (peek() == '#')
			{
				skip_comment();
				continue;
			}
			advance();
			value += c;
		}
		
		size_t start = value.find_first_not_of(" \t\n\r");
		size_t end = value.find_last_not_of(" \t\n\r");
		
		if (start == std::string::npos) value = "";
		else value = value.substr(start, end - start + 1);
		
		return {TokenType::TEXT, value, start_line, start_col};
	};
	
	bool inside_tag = false;
	
	while (peek() != '\0')
	{
		skip_comment();
		
		if (!inside_tag && std::isspace(peek()))
		{
			advance();
			continue;
		}
		
		int start_line = line;
		int start_col = column;
		
		if (peek() == '<')
		{
			advance();
			
			if (peek() == '/')
			{
				advance();
				skip_whitespace_in_tag();
				
				if (!std::isalpha(peek())) throw TdrError(line, column, "Expected valid tag name after '</'");
				
				tokens.push_back({TokenType::TAG_END_OPEN, "", start_line, start_col});
				tokens.push_back(read_identifier(line, column));
				inside_tag = true;
				
			}
			else
			{
				skip_whitespace_in_tag();

				if (!std::isalpha(peek())) throw TdrError(line, column, "Expected valid tag name after '<'");
	
				tokens.push_back({TokenType::TAG_OPEN, "", start_line, start_col});
				tokens.push_back(read_identifier(line, column));
				inside_tag = true;
			}
		}
		else if (inside_tag && peek() == '>')
		{
			advance();
			tokens.push_back({TokenType::TAG_CLOSE, "", start_line, start_col});
			inside_tag = false;
		}
		else if (inside_tag && peek() == '/')
		{
			advance();
			if (peek() == '>')
			{
				advance();
				tokens.push_back({TokenType::TAG_SELF_CLOSE, "", start_line, start_col});
				inside_tag = false;
			}
			else throw TdrError(line, column, "Expected '>' after '/'");
		}
		else if (inside_tag && peek() == '=')
		{
			advance();
			tokens.push_back({TokenType::EQUALS, "", start_line, start_col});
		}
		else if (inside_tag && (peek() == '"' || peek() == '\''))
		{
			advance();
			tokens.push_back(read_string(start_line, start_col));
		}
		else if (inside_tag && std::isalpha(peek()))
		{
			tokens.push_back(read_identifier(line, column));
		}
		else if (inside_tag && std::isspace(peek()))
		{
			skip_whitespace_in_tag();
		}
		else if (!inside_tag && peek() != '\0')
		{
			Token text = read_text(start_line, start_col);

			if (!text.value.empty()) tokens.push_back(text);
		}
		else throw TdrError(line, column, std::string("Unexpected character '") + peek() + "'");
	}
	tokens.push_back((Token){TokenType::END_OF_FILE, "", line, column});
	return tokens;
}


void print_tokens(const std::vector<Token>& tokens)
{
	std::cout << STYLE_BOLD << COLOR_BRIGHT_CYAN << "=== TOKENS ===" << COLOR_RESET << std::endl;
	
	for (const Token& token : tokens)
	{
		std::cout << COLOR_BRIGHT_BLACK 
				<< std::setw(4) << std::right << token.line 
				<< ":" 
				<< std::setw(3) << std::left << token.column 
				<< COLOR_RESET;
		
		std::cout << " " << COLOR_YELLOW 
				<< std::setw(16) << std::left << token.type 
				<< COLOR_RESET;
		
		if (!token.value.empty()) {
			std::cout << " " << COLOR_GREEN 
					<< "\"" << token.value << "\"" 
					<< COLOR_RESET;
		}
		
		std::cout << std::endl;
	}
	
	std::cout << STYLE_BOLD << COLOR_BRIGHT_CYAN  << "=== " << tokens.size() << " tokens ===" << COLOR_RESET << std::endl;
}

const std::string getTokenContent(Token& tok)
{
	switch (tok.type)
	{
		case TokenType::TAG_OPEN: return "<";
		case TokenType::TAG_END_OPEN: return "</";
		case TokenType::TAG_CLOSE: return ">";
		case TokenType::TAG_SELF_CLOSE: return "/>";
		case TokenType::EQUALS: return "=";
		case TokenType::END_OF_FILE: return "End Of File";
		case TokenType::IDENTIFIER: return "identifier: " + std::string("\"") + tok.value + std::string("\"");
		case TokenType::STRING: return "string: " + std::string("\"") + tok.value + std::string("\"");
		case TokenType::TEXT: return "text: " + std::string("\"") + tok.value + std::string("\"");
	}
}

}