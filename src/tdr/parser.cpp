#include "tdr/parser.hpp"
#include <iostream>
#include "colors.hpp"
#include <memory>
#include <functional>

namespace sceneIO::tdr {

Node parser(std::vector<Token>& list, ErrorCollector& errors)
{
	Node root;

	size_t cursor = 0;

	auto peek = [&]() -> Token&
	{
		return list[cursor];
	};

	auto last = [&]() -> Token&
	{
		if (cursor > 0) return list[cursor - 1];
		return list[cursor];
	};

	auto advance = [&](const std::unique_ptr<Node>& node) -> Token&
	{
		if (cursor >= list.size()) throw TdrError("Internal TDR parser error: cursor overflow");
		
		Token& current = list[cursor];
		
		if (node) node->tokens_.push_back(current);
		
		cursor++;
		return current;
	};

	auto skip_close_tag_unwanted_token = [&]()
	{
		while (peek().type != TokenType::IDENTIFIER
			&& peek().type != TokenType::TAG_CLOSE
			&& peek().type != TokenType::TAG_SELF_CLOSE
			&& peek().type != TokenType::END_OF_FILE)
		{
			advance(nullptr);
		}
	};

	std::function<std::unique_ptr<Node>()> parseNode = [&]() -> std::unique_ptr<Node>
	{
		std::cout << "OEOE" << std::endl;
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
			AttributeInfos attr;
			attr.attr_line = peek().line;
			attr.attr_column = peek().column;
			
			if (res->attributes_.find(propertyName) != res->attributes_.end())
				errors.report(TdrError(peek().line, peek().column, "Duplicated attribute '" + propertyName + "'"));
			
			res->attributes_[propertyName] = attr;

			advance(res);

			if (peek().type == TokenType::END_OF_FILE) return eofError();
			else if (peek().type == TokenType::EQUALS)
			{
				advance(res);

				if (peek().type == TokenType::END_OF_FILE) return eofError();
				else if (peek().type == TokenType::STRING)
				{
					attr.content_line = peek().line;
					attr.content_column = peek().column + 1;
					attr.content = peek().value;
					res->attributes_[propertyName] = attr;
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
		else
		{
			if (peek().type == TokenType::TAG_CLOSE)
				advance(res);
			else
				errors.report(TdrError(last().line, last().column, "Unclosed tag '" + res->identifier_ + "' inside a tag. Close it with '>' or '/>'"));;

			while (peek().type != TokenType::END_OF_FILE)
			{
				if (peek().type == TokenType::TEXT)
				{
					if (res->text_.empty()) res->text_ = peek().value;
					else errors.report(TdrError(peek().line, peek().column, "Multiple text blocks not allowed (text content must be in a single block)"));
					advance(res);
				}
				else if (peek().type == TokenType::TAG_END_OPEN)
				{
					advance(res);

					if (peek().type != TokenType::IDENTIFIER) errors.report(TdrError(peek().line, peek().column, "Invalid end of tag. Expected '</" + res->identifier_ + ">'"));
					skip_close_tag_unwanted_token();

					if (peek().type == TokenType::END_OF_FILE) return eofError();
					else if (peek().type == TokenType::TAG_CLOSE || peek().type == TokenType::TAG_SELF_CLOSE)
					{
						errors.report(TdrError(peek().line, peek().column, "Invalid close tag. Expected '</" + res->identifier_ + ">'"));
						advance(res);
						return res;
					}

					if (peek().value == res->identifier_)
					{
						advance(res);
						if (peek().type == TokenType::TAG_CLOSE)
						{
							advance(res);
							return res;
						}
						errors.report(TdrError(peek().line, peek().column, "Invalid end of tag. Expected '</" + res->identifier_ + ">'"));
						
						while (peek().type != TokenType::TAG_CLOSE && peek().type != TokenType::END_OF_FILE)
						{
							if (peek().type == TokenType::TAG_SELF_CLOSE)
							{
								errors.report(TdrError(peek().line, peek().column, "Found '/>' instead of '>'"));
								advance(res);
								return res;
							}
							else if (peek().type == TokenType::TAG_OPEN || peek().type == TokenType::TAG_END_OPEN)
							{
								errors.report(TdrError(peek().line, peek().column, "Unclosed tag '</" + res->identifier_ + "'"));
								return res;
							}
							else
								advance(nullptr);
						}
						
						if (peek().type == TokenType::TAG_CLOSE)
						{
							advance(res);
							return res;
						}
						else return eofError();
					}
					else
					{
						if (cursor < 2) throw TdrError("Internal TDR parser error: cursor underflow protection");
						cursor--;
						errors.report(TdrError(peek().line, peek().column, "Unclosed tag '<" + res->identifier_ + ">'"));
						return res;
					}
				}
				else if (peek().type == TokenType::TAG_OPEN)
				{
					std::unique_ptr<Node> child = parseNode();

					if (child == nullptr) return nullptr;

					res->children_.push_back(std::move(*child));
				}
				else
				{
					errors.report(TdrError(peek().line, peek().column, "Unexpected token '" + getTokenContent(peek()) + "'."));
					advance(nullptr);
				}
			}
		}
		
		return res;
	};

	while (peek().type != TokenType::END_OF_FILE)
	{
		if (peek().type == TokenType::TAG_OPEN)
		{
			std::unique_ptr<Node> child = parseNode();

			if (child) root.children_.push_back(std::move(*child));
		}
		else
		{
			errors.report(TdrError(peek().line, peek().column, "Unexpected token '" + getTokenContent(peek()) + "'."));
			advance(nullptr);
		}
	}

	return root;

}

void Node::print(int nest) const
{
	COLORS_INIT();
	std::string indent(nest * 2, ' ');
	
	std::cout << indent << COLOR_CYAN << "<" << identifier_ << COLOR_RESET;
	
	for (const auto& [key, value] : attributes_)
	{
		std::cout << " " << COLOR_YELLOW << key << COLOR_RESET << "=" << COLOR_GREEN << "\"" << value.content << "\"" << COLOR_RESET;
	}

	if (children_.empty() && text_.empty())
	{
		std::cout << COLOR_CYAN << " />" << COLOR_RESET << std::endl;
	}
	else
	{
		std::cout << COLOR_CYAN << ">" << COLOR_RESET;

		bool hasContent = !text_.empty();
		if (hasContent)
		{
			std::cout << COLOR_WHITE << text_ << COLOR_RESET;
		}
		
		if (!children_.empty())
		{
			if (hasContent) std::cout << std::endl;
			else std::cout << std::endl;
			
			for (const Node& child : children_)
			{
				child.print(nest + 1);
			}
			std::cout << indent;
		}
		
		if (!children_.empty())
			std::cout << COLOR_CYAN << "</" << identifier_ << ">" << COLOR_RESET << std::endl;
		else
			std::cout << COLOR_CYAN << "</" << identifier_ << ">" << COLOR_RESET << std::endl;
	}
}


const std::pair<uint64_t, uint64_t> Node::getTextBeginPos() const
{
	auto pos = getNodeBeginPos();
	for (const auto& token : tokens_)
	{
		if (token.type == TokenType::TEXT)
		{
			pos.first = token.line;
			pos.second = token.column;
			break;
		}
	}
	return pos;
}

const std::pair<uint64_t, uint64_t> Node::getNodeBeginPos() const
{
	size_t i = 0;

	while (i < tokens_.size())
	{
		if (tokens_[i].type == TokenType::IDENTIFIER)
			return { tokens_[i].line, tokens_[i].column };
		i++;
	}
	return { UINT64_MAX, UINT64_MAX };
}

}