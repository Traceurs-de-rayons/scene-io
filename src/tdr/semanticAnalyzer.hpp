#pragma once

#include "tdr/parser.hpp"
#include "tdr/error.hpp"

namespace sceneIO::tdr {

template <typename T>
bool isValidValue(const std::string& s)
{
	static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
	
	if (s.empty()) return false;

	T value;
	auto [ptr, ec] = std::from_chars(
		s.data(),
		s.data() + s.size(),
		value
	);

	return ec == std::errc() && ptr == s.data() + s.size();
}

void semanticAnalyzer(Node& ast, ErrorCollector& errors);

}