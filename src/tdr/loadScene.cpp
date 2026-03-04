#include "tdr/LanguageService.hpp"
#include "tdr/loadScene.hpp"

#include <charconv>

namespace sceneIO::tdr {

std::vector<sceneIO::tdr::Node>::const_iterator SceneLoader::getChildElement(const Node& n, const std::string& name)
{
	return find_if(	n.getChildren().begin(),
					n.getChildren().end(),
					[&](const Node& n) { return n.getIdentifier() == name; }); 
}

static float getColorChar(const std::string& s)
{
	int value = 0;
	auto [ptr, ec] = std::from_chars(
		s.data(),
		s.data() + s.size(),
		value
	);
	return value / 255.0f;
}

static float getFloat(const std::string& s)
{
	float value = 0.0;
	auto [ptr, ec] = std::from_chars(
		s.data(),
		s.data() + s.size(),
		value
	);
	return value;
}

vec3 getColor(const std::string& s)
{
	if (s.empty()) return false;

	if (s.size() == 7 && s[0] == '#')
	{
		int r = std::stoi(s.substr(1, 2), nullptr, 16);
		int g = std::stoi(s.substr(3, 2), nullptr, 16);
		int b = std::stoi(s.substr(5, 2), nullptr, 16);

		return vec3(r / 255.0f, g / 255.0f, b / 255.0f);
	}

	std::vector<std::string> parts = cu::string::split(s, ',');
	if (parts.size() == 3)
		return vec3(getColorChar(parts[0]), getColorChar(parts[1]), getColorChar(parts[2]));
	else
	{
		parts = cu::string::split(s, ' ');
		if (parts.size() == 3)
			return vec3(getFloat(parts[0]), getFloat(parts[1]), getFloat(parts[2]));
	}
	return vec3(0);
}


void SceneLoader::debugTextures() const
{
	cu::logger::info("=== Textures (" + std::to_string(scene_.textures_.size()) + ") ===");
	for (const auto& [key, tex] : scene_.textures_)
	{
		std::string msg = "[Texture] name=\"" + tex.name + "\"";
		if (!tex.label.empty())
			msg += " label=\"" + tex.label + "\"";

		if (std::holds_alternative<Texture::FromFile>(tex.data))
		{
			const auto& ff = std::get<Texture::FromFile>(tex.data);
			msg += " type=filepath path=\"" + ff.path + "\"";
			if (ff.state == TextureState::Loaded || ff.state == TextureState::Uploaded)
				msg += " size=" + std::to_string(ff.width) + "x" + std::to_string(ff.height)
				     + " channels=" + std::to_string(ff.channels);
		}
		else if (std::holds_alternative<Texture::CheckerLocal>(tex.data))
		{
			const auto& cl = std::get<Texture::CheckerLocal>(tex.data);
			msg += " type=checker_local"
			     + std::string(" even=(") + std::to_string(cl.even.x) + "," + std::to_string(cl.even.y) + "," + std::to_string(cl.even.z) + ")"
			     + " odd=("  + std::to_string(cl.odd.x)  + "," + std::to_string(cl.odd.y)  + "," + std::to_string(cl.odd.z)  + ")"
			     + " scale=" + std::to_string(cl.scale);
		}
		else if (std::holds_alternative<Texture::CheckerGlobal>(tex.data))
		{
			const auto& cg = std::get<Texture::CheckerGlobal>(tex.data);
			msg += " type=checker_global"
			     + std::string(" even=(") + std::to_string(cg.even.x) + "," + std::to_string(cg.even.y) + "," + std::to_string(cg.even.z) + ")"
			     + " odd=("  + std::to_string(cg.odd.x)  + "," + std::to_string(cg.odd.y)  + "," + std::to_string(cg.odd.z)  + ")"
			     + " scale=" + std::to_string(cg.scale);
		}

		cu::logger::info(msg);
	}
	cu::logger::info("=== End Textures ===");
}


void SceneLoader::loadTextures()
{
	auto it = getChildElement(ast_, "textures");
	
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
			Texture::FromFile tmp = {};

			tmp.path = getChildElement(texture, "path")->getText();
			tex.data = std::move(tmp);
		}
		else if (type == "checker_local")
		{
			Texture::CheckerLocal tmp = {};
			tmp.odd = getColor(getChildElement(texture, "odd")->getText());
			tmp.even = getColor(getChildElement(texture, "even")->getText());
			tmp.scale = getFloat(getChildElement(texture, "scale")->getText());
			tex.data = std::move(tmp);
		}
		else if (type == "checker_global")
		{
			Texture::CheckerGlobal tmp = {};
			tmp.odd = getColor(getChildElement(texture, "odd")->getText());
			tmp.even = getColor(getChildElement(texture, "even")->getText());
			tmp.scale = getFloat(getChildElement(texture, "scale")->getText());
			tex.data = std::move(tmp);
		}
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
	debugTextures();

	return std::move(scene_);
}

}
