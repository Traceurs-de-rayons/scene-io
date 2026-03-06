#pragma once

#include "tdr/parser.hpp"
#include "scene-core.hpp"

#include <exception>

namespace sceneIO::tdr {

class SceneLoader
{

private:
	std::string path_;
	Scene scene_;
	Node ast_;
	ErrorCollector errors_;
	

	std::vector<sceneIO::tdr::Node>::const_iterator getChildElement(const Node& n, const std::string& name);

	void loadTextures();
	void debugTextures() const;
	void loadMaterials();
	void loadAssets();
	void loadCameras();
	void loadLights();

public:
	Scene load(const std::string& path);

};

}