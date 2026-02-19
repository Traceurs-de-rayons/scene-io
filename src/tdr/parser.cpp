#include "tdr/parser.hpp"
#include <memory>

namespace sceneIO::tdr {

Node parser(std::vector<Token>& list, ErrorCollector& errors)
{
	Node root;

	size_t cursor = 0;

	auto peek = [&]() -> Token&
	{
		return list[cursor];
	};

	auto advance = [&](const std::unique_ptr<Node>& node) -> Token&
	{
		if (node) node->tokens_.push_back(list[cursor]);
		if (list.size() < cursor) throw TdrError("Internal TDR parser error");
		cursor++;
		return list[cursor];
	};

	auto parseNode = [&]() -> std::unique_ptr<Node>
	{
		auto eofError = [&]() -> std::unique_ptr<Node>
		{
			errors.report(TdrError(peek().line, peek().column, "Unexpected end of file"));
			return nullptr;
		};

		auto res = std::make_unique<Node>();

		advance(res);

		if (peek().type == TokenType::END_OF_FILE) return eofError();
		else if (peek().type != TokenType::IDENTIFIER) errors.report(TdrError(peek().line, peek().column, "Tag identifier expected"));
		else res->identifier_ = peek().value;

		advance(res);

		while (peek().type == TokenType::IDENTIFIER)
		{
			std::string& propertyName = peek().value;

			advance(res);

			if (peek().type == TokenType::END_OF_FILE) return eofError();
			else if (peek().type == TokenType::EQUALS)
			{
				advance(res);

				if (peek().type == TokenType::END_OF_FILE) return eofError();
				else if (peek().type == TokenType::STRING)
				{
					res->attributes_[propertyName] = peek().value;
					advance(res);
				}
				else errors.report(TdrError(peek().line, peek().column, "Expected string value after '=' (did you forget quotes?)"));
			}
		}

		if (peek().type == TokenType::END_OF_FILE) return eofError();
		else if (peek().type == TokenType::TAG_SELF_CLOSE)
		{
			advance(res);
			return res;
		}
		else if (peek().type == TokenType::TAG_CLOSE)
		{
			advance(res);

			while (peek().type != TokenType::END_OF_FILE)
			{
				if (peek().type == TokenType::TEXT)
				{
					if (res->text_.empty()) res->text_ = peek().value;
					else errors.report(TdrError(peek().line, peek().column, "Unexpected text. (splitted content not allowed)"));
				}
				else if (peek().type == TokenType::TAG_CLOSE) // END TAG
				{
					advance(res);
					if (peek().type == TokenType::IDENTIFIER)
					{
						if (peek().value == res->identifier_)
						{
							advance(res);
							if (peek().type == TokenType::TAG_CLOSE)
							{
								advance(res);
								return res;
							}
							else errors.report(TdrError(peek().line, peek().column, "Invalid end of tag. Expected '</" + res->identifier_ + ">'"));
						}
						else
						{
							errors.report(TdrError(peek().line, peek().column, "Unexpected identifier '"+ peek().value +"'. Expected '" + res->identifier_ + "'"));
						}
						
					}
				}
			}

			// recup text + recursivité
		}
		else errors.report(TdrError(peek().line, peek().column, "Unexpected token '" + getTokenContent(peek()) + "' inside a tag. Close it with '>' or '/>'"));


	};

	while (peek().type != TokenType::END_OF_FILE)
	{
		if (peek().type == TokenType::TAG_OPEN)
		{

		}
	}

}

}