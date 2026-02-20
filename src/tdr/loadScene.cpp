#include "tdr/LanguageService.hpp"
#include "tdr/loadScene.hpp"

namespace sceneIO::tdr {



void loadSceneFromFile(Scene& scene, const std::string& path)
{
	ParseResult res = SceneLanguageService::parse_file(path);
	
	for (TdrError e : res.errors)
	{
		cu::logger::error(e.getError());
	}

	res.ast.print();

}

Scene loadSceneFromFile(const std::string& path)
{
	Scene s;
	loadSceneFromFile(s, path);
	return s;
}

}
