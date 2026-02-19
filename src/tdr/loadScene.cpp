#include "tdr/loadScene.hpp"
#include "tdr/parser.hpp"
#include "tdr/lexer.hpp"
#include "tdr/error.hpp"

namespace sceneIO::tdr {



void loadSceneFromFile(Scene& scene, const std::string& path)
{
	std::ifstream in(path, std::ios_base::in);

	try
	{
		if (!in.is_open()) throw TdrError("Cannot open file");
	
		auto tokens = lexer(in);
		print_tokens(tokens);

		ErrorCollector errors;

		Node root = parser(tokens, errors);

		

		for (TdrError e : errors.get_errors())
		{
			e.location.filepath = path;
			cu::logger::error(TdrError(e.location, e.getMessage()).what());
		}
	}
	catch (TdrError& e)
	{
		e.location.filepath = path;
		throw TdrError(e.location, e.getMessage());
	}
	
	
	
		
}

Scene loadSceneFromFile(const std::string& path)
{
	Scene s;
	loadSceneFromFile(s, path);
	return s;
}

}
