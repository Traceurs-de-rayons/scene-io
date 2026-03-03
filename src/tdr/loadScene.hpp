#pragma once

#include "tdr/parser.hpp"
#include "scene-core.hpp"

#include <exception>

namespace sceneIO::tdr {

class SceneLoader
{
	private:
		Scene scene_;
		Node ast_;
		ErrorCollector errors_;

		void loadTextures();

	public:
		Scene load(const std::string& path);
};

}