#pragma once

#include "scene-core.hpp"
#include <exception>

namespace sceneIO::tdr {

	void loadSceneFromFile(Scene& scene, const std::string& path);
	Scene loadSceneFromFile(const std::string& path);

}