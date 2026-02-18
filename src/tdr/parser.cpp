
#include "tdr/parser.hpp"

namespace sceneIO::tdr {

Node parser(std::vector<Token>& list, ErrorCollector& errors)
{
	Node root;

	size_t cursor = 0;

	auto peek = [&]() -> Token*
	{
		if (list.size() >= cursor) return &list[cursor];
		return nullptr;
	};

	auto advance = [&]() -> Token&
	{
		if (list.size() < cursor) throw TdrError("Internal TDR parser error");
		cursor++;
		return list[cursor];
	};

	auto current = [&]() -> Token&
	{
		return list[cursor];
	};

	auto parseNode = [&]() -> Node*
	{
		auto eofError = [&]() -> Node*
		{
			errors.report(TdrError(current().line, current().column, "Invalid tag at the end of the file"));
			return nullptr;
		};

		Node res;

		advance();

		if (!peek()) return eofError();
		advance();

		if (current().type != TokenType::IDENTIFIER) errors.report(TdrError(current().line, current().column, "Tag identifier expected"));
		else res.identifier_ = current().value;

		if (!peek()) return eofError();
		advance();

		while (current().type == TokenType::IDENTIFIER)
		{
			std::string& propertyName = current().value;

			if (!peek()) return eofError();
			advance();

			if (current().type == TokenType::EQUALS)
			{
				if (!peek()) return eofError();
				advance();

				if (current().type == TokenType::STRING)
				{
					res.attributes_[propertyName] = current().value;
					if (!peek()) return eofError(); // Au moi de demain matin, remet le node EOF en fait, ca permet d'eviter tous les !peek (je pense)
					advance();
				}
				else
				{

				}
			}
		}


	};

	while (peek())
	{
		if (current().type == TokenType::TAG_OPEN)
		{

		}
	}

}

}