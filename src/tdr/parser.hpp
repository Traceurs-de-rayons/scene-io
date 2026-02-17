#pragma once

#include "scene-core.hpp"
#include <exception>

namespace sceneIO::tdr {

	class TdrParseError : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	void parseTdr(Asset& asset, const std::string& path);
	Asset parseTdr(const std::string& path);


	namespace tdr {

		

	}

}