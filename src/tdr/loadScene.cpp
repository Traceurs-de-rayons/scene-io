#include "tdr/LanguageService.hpp"
#include "tdr/loadScene.hpp"

namespace sceneIO::tdr {

void SceneLoader::loadTextures()
{
	auto it = find_if(ast_.getChildren().begin(), ast_.getChildren().end(),
					[&](const Node& n) { return n.getIdentifier() == "textures"; }); 
	
	if (it == ast_.getChildren().end()) return ;

	const Node& textures = *it;

	for (const Node& texture : textures.getChildren())
	{
		const auto& tex_attr = texture.getAttributes();
		const AttributeInfos& name = tex_attr.find("name")->second;
		if (scene_.textures_.find(name.content) != scene_.textures_.end())
		{
			errors_.report(TdrError(name.content_line, name.content_column, 2, "Duplicated texture name, it will be ignored"));
			continue;
		}

		Texture& tex = scene_.textures_[name.content];

		tex.name = name.content;

		auto label = tex_attr.find("label");
		if (label != tex_attr.end()) tex.label = label->second.content;

		const std::string& type = tex_attr.find("type")->second.content;
	
		if (type == "filepath")
		{

		}
		else if (type == "checker_local")
		{

		}
		else if (type == "checker_global")
		{

		}

		// struct FromFile
		// {
		// 	std::string path;
		// 	TextureState state;
		// 	std::unique_ptr<uint8_t[]> data;
		// 	int width = 0;
		// 	int height = 0;
		// 	int channels = 0;
		// };

		// struct CheckerLocal
		// {
		// 	cu::math::vec3 even;
		// 	cu::math::vec3 odd;
		// 	float scale;
		// };

		// struct CheckerGlobal
		// {
		// 	cu::math::vec3 even;
		// 	cu::math::vec3 odd;
		// 	float scale;
		// };

		// std::variant<FromFile, CheckerLocal, CheckerGlobal> data;
	}
}

Scene SceneLoader::load(const std::string& path)
{
	ParseResult res = SceneLanguageService::parse_file(path);
	bool	has_error = false;

	for (TdrError e : res.errors)
	{
		if (e.getErrorLevel() == 1)
		{
			has_error = true;
			cu::logger::error(e.getError());
		}
		else if (e.getErrorLevel() == 2) cu::logger::warn(e.getError());
		else cu::logger::info(e.getError());
	}

	if (has_error) throw std::runtime_error("Cannot open the scene with an error present on the file.");

	ast_ = std::move(res.ast);

	// res.ast.print();

	loadTextures();

	return std::move(scene_);
}

}
