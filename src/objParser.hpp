#pragma once

#include "scene-core.hpp"
#include <exception>

namespace sceneIO::parser {

	class ObjParseError : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	void parseObj(Asset& asset, const std::string& path);
	Asset parseObj(const std::string& path);

}