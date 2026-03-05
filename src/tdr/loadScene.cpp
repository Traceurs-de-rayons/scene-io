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


void SceneLoader::loadMaterials()
{
	auto it = getChildElement(ast_, "materials");
	
	if (it == ast_.getChildren().end()) return ;

	const Node& materials = *it;

	for (const Node& material : materials.getChildren())
	{
		const auto& mat_attr = material.getAttributes();
		const AttributeInfos& name = mat_attr.find("name")->second;
		if (scene_.materials_.find(name.content) != scene_.materials_.end())
		{
			errors_.report(TdrError(name.content_line, name.content_column, 2, "Duplicated material name, it will be ignored"));
			continue;
		}

		Material& mat = scene_.materials_[name.content];

		mat.name = name.content;

		auto label = mat_attr.find("label");
		if (label != mat_attr.end()) mat.label = label->second.content;


		for (const Node& prop : material.getChildren())
		{
			const auto& prop_attr = prop.getAttributes();
			const auto& type_it = prop_attr.find("type");

			if (type_it != prop_attr.end() && type_it->second.content == "texture")
			{
				if (scene_.textures_.find(prop.getText()) == scene_.textures_.end())
				{
					const auto& pos = prop.getTextBeginPos();
					errors_.report(TdrError(pos.first, pos.second, 2, "Unknown texture reference '"+ prop.getText() +"' in material '"+ mat.name +"' (property: "+ prop.getIdentifier() +")."));
				}
			}

			if (prop.getIdentifier() == "albedo")
			{
				const std::string& type = type_it->second.content;
				if (type == "texture")
					mat.albedo = MaterialParam<cu::math::vec3>{ prop.getText() };
				else
					mat.albedo = MaterialParam<cu::math::vec3>{ getColor(prop.getText()) };
			}
			else if (prop.getIdentifier() == "metallic")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.metallic = MaterialParam<float>{ prop.getText() };
				else
					mat.metallic = MaterialParam<float>{ getFloat(prop.getText()) };
			}
			else if (prop.getIdentifier() == "roughness")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.roughness = MaterialParam<float>{ prop.getText() };
				else
					mat.roughness = MaterialParam<float>{ getFloat(prop.getText()) };
			}
			else if (prop.getIdentifier() == "transmission")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.transmission = MaterialParam<float>{ prop.getText() };
				else
					mat.transmission = MaterialParam<float>{ getFloat(prop.getText()) };
			}
			else if (prop.getIdentifier() == "ambient_occlusion")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.ambient_occlusion = MaterialParam<float>{ prop.getText() };
				else
					mat.ambient_occlusion = MaterialParam<float>{ getFloat(prop.getText()) };
			}
			else if (prop.getIdentifier() == "roughness")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.roughness = MaterialParam<float>{ prop.getText() };
				else
					mat.roughness = MaterialParam<float>{ getFloat(prop.getText()) };
			}
			else if (prop.getIdentifier() == "emission_strength")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.emission_strength = MaterialParam<float>{ prop.getText() };
				else
					mat.emission_strength = MaterialParam<float>{ getFloat(prop.getText()) };
			}
			else if (prop.getIdentifier() == "emission_color")
			{
				const std::string& type = type_it->second.content;

				if (type == "texture")
					mat.emission_color = MaterialParam<cu::math::vec3>{ prop.getText() };
				else
					mat.emission_color = MaterialParam<cu::math::vec3>{ getColor(prop.getText()) };
			}
			else if (prop.getIdentifier() == "ior")
			{
				mat.ior = getFloat(prop.getText());
			}
			else if (prop.getIdentifier() == "texture_scale")
			{
				mat.texture_scale = getFloat(prop.getText());
			}
			else if (prop.getIdentifier() == "normal_map")
			{
				mat.normal_map = prop.getText();
			}
			else if (prop.getIdentifier() == "normal_intensity")
			{
				mat.normal_intensity = getFloat(prop.getText());
			}
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
	// debugTextures();
	loadMaterials();

	for (TdrError e : errors_.get_errors())
	{
		e.location.filepath = path;
		if (e.getErrorLevel() == 1)
		{
			has_error = true;
			cu::logger::error(e.getError());
		}
		else if (e.getErrorLevel() == 2) cu::logger::warn(e.getError());
		else cu::logger::info(e.getError());
	}

	if (has_error) throw std::runtime_error("Cannot open the scene with an error present on the file.");

	return std::move(scene_);
}

}
