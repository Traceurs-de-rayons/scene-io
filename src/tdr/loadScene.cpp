#include "tdr/LanguageService.hpp"
#include "tdr/loadScene.hpp"
#include "objParser.hpp"

#include <charconv>
#include <iostream>
#include <vector>

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

static int getInt(const std::string& s)
{
	int value = 0.0;
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

vec3 getVec3(const std::string& s)
{
	auto parts = cu::string::split(s, ' ');

	return {getFloat(parts[0]), getFloat(parts[1]), getFloat(parts[1])};
}

quat getQuat(const std::string& s)
{
	throw std::runtime_error("J'ai eu la flemme d'implementer le parsing des quaternions donc je throw. A un moment je le gererai hein");
	auto parts = cu::string::split(s, ' ');
	return quat(getFloat(parts[3]), getFloat(parts[0]), getFloat(parts[1]), getFloat(parts[2]));
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

	if (it == ast_.getChildren().end())
		return ;

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


void SceneLoader::loadAssets()
{
	auto it = getChildElement(ast_, "assets");

	if (it == ast_.getChildren().end())
		return ;

	const Node& assets = *it;

	for (const Node& asset_node : assets.getChildren())
	{
		const auto& asset_attr = asset_node.getAttributes();
		const AttributeInfos& name = asset_attr.find("name")->second;
		if (scene_.assets_.find(name.content) != scene_.assets_.end())
		{
			errors_.report(TdrError(name.content_line, name.content_column, 2, "Duplicated asset name, the asset will be ignored"));
			continue;
		}

		Asset& asset = scene_.assets_[name.content];

		asset.name_ = name.content;

		auto label = asset_attr.find("label");
		if (label != asset_attr.end()) asset.label_ = label->second.content;

		const std::string& type = asset_attr.find("type")->second.content;
		if (type == "object")
		{
			const auto& obj = getChildElement(asset_node, "object");
			const std::string& obj_type = obj->getAttributes().find("type")->second.content;
			sceneIO::parser::ObjErrorCollector obj_errors;

			if (obj_type == "external")
			{
				const std::string& path = obj->getAttributes().find("path")->second.content;
				sceneIO::parser::parseObj(asset, path, obj_errors);
			}
			else
			{
				std::istringstream ss(obj->getText());
				const auto pos = obj->getTextBeginPos();

				sceneIO::parser::parseObj(asset, ss, obj_errors, pos.first, pos.second);
				obj_errors.setFilePath(path_);
			}

			bool obj_has_error = false;
			for (const sceneIO::parser::ObjError& e : obj_errors.getErrors())
			{
				obj_has_error = true;
				cu::logger::error(e.getError());
			}

			if (obj_has_error) throw std::runtime_error("Cannot open the scene with an error present on the file.");
		}
		else if (type == "primitive")
		{
			Asset::PrimitiveData tmp = {};

			const auto& prim = getChildElement(asset_node, "primitive");
			const std::string& prim_type = prim->getAttributes().find("type")->second.content;

			if (prim_type == "plane")
			{
				Asset::PrimitiveData::Plane tmp_prim = {};

				tmp_prim.normal = getFloat(getChildElement(*prim, "normal")->getText());
				tmp.primitive = std::move(tmp_prim);
			}
			else if (prim_type == "sphere")
			{
				Asset::PrimitiveData::Sphere tmp_prim = {};

				tmp_prim.radius = getFloat(getChildElement(*prim, "radius")->getText());
				tmp.primitive = std::move(tmp_prim);
			}
			else if (prim_type == "cylinder")
			{
				Asset::PrimitiveData::Cylinder tmp_prim = {};

				tmp_prim.radius = getFloat(getChildElement(*prim, "radius")->getText());
				tmp_prim.height = getFloat(getChildElement(*prim, "height")->getText());
				tmp.primitive = std::move(tmp_prim);
			}
			else if (prim_type == "cone")
			{
				Asset::PrimitiveData::Cone tmp_prim = {};

				tmp_prim.radius = getFloat(getChildElement(*prim, "radius")->getText());
				tmp_prim.height = getFloat(getChildElement(*prim, "height")->getText());
				tmp.primitive = std::move(tmp_prim);
			}
			else if (prim_type == "hyperboloid")
			{
				Asset::PrimitiveData::Hyperboloid tmp_prim = {};

				tmp_prim.height = getFloat(getChildElement(*prim, "height")->getText());
				tmp_prim.a = getFloat(getChildElement(*prim, "a")->getText());
				tmp_prim.b = getFloat(getChildElement(*prim, "b")->getText());
				tmp_prim.c = getFloat(getChildElement(*prim, "c")->getText());
				tmp_prim.shape = getFloat(getChildElement(*prim, "shape")->getText());
				tmp.primitive = std::move(tmp_prim);
			}

			asset.content_ = std::move(tmp);
		}
		else if (type == "instance")
		{
			Asset::InstanceData tmp = {};

			tmp.parent_name = getChildElement(asset_node, "parent")->getAttributes().find("ref")->second.content;

			asset.content_ = std::move(tmp);
		}

		const auto& mat = getChildElement(asset_node, "material");
		if (mat != asset_node.getChildren().end())
		{
			asset.material_ = mat->getAttributes().find("ref")->second.content;
		}

		const auto& transform = getChildElement(asset_node, "transform");
		if (transform != asset_node.getChildren().end())
		{
			const auto& transform_pos = getChildElement(*transform, "position");
			if (transform_pos != transform->getChildren().end())
			{
				asset.transform_.setTranslation(getVec3(transform_pos->getText()));
			}

			const auto& transform_rotation = getChildElement(*transform, "rotation");
			if (transform_rotation != transform->getChildren().end())
			{
				std::string rot_type = transform_rotation->getAttributes().find("type")->second.content;

				if (rot_type == "euler")
				{
					auto angles = getVec3(transform_rotation->getText());
					asset.transform_.setEulerAngles(angles.x, angles.y, angles.z);
				}
				else
				{
					asset.transform_.setRotation(getQuat(transform_rotation->getText()));
					throw std::runtime_error("J'ai eu la flemme d'implementer le parsing des quaternions donc je throw. A un moment je le gererai hein");
				}
			}

			const auto& transform_scale = getChildElement(*transform, "scale");
			if (transform_scale != transform->getChildren().end())
			{
				asset.transform_.setScale(getVec3(transform_scale->getText()));
			}
		}
	}
}

void SceneLoader::loadCameras()
{
	auto it = getChildElement(ast_, "cameras");

	if (it == ast_.getChildren().end()) return;

	const Node& cameras = *it;

	for (const Node& camera_node : cameras.getChildren())
	{
		const auto& cam_attr = camera_node.getAttributes();
		const AttributeInfos& name = cam_attr.find("name")->second;

		if (scene_.cameras_.find(name.content) != scene_.cameras_.end())
		{
			errors_.report(TdrError(name.content_line, name.content_column, 2, "Duplicated camera name, it will be ignored"));
			continue;
		}

		Camera& cam = scene_.cameras_[name.content];
		cam.name = name.content;

		auto label = cam_attr.find("label");
		if (label != cam_attr.end()) cam.label = label->second.content;

		const std::string& projection = cam_attr.find("projection")->second.content;

		if (projection == "perspective")
		{
			Camera::Perspective persp = {};

			const Node& fov_node = *getChildElement(camera_node, "fov");
			const std::string& fov_mode = fov_node.getAttributes().find("mode")->second.content;

			if (fov_mode == "physical")
			{
				Camera::Perspective::PhysicalFOV phys = {};

				const std::string& sensor_fit_str = fov_node.getAttributes().find("sensor_fit")->second.content;
				phys.sensor_fit = (sensor_fit_str == "vertical") ? Camera::Perspective::SensorFit::VERTICAL : Camera::Perspective::SensorFit::HORIZONTAL;

				phys.focal_length = getFloat(getChildElement(fov_node, "focal_length")->getText());
				phys.sensor_width  = getFloat(getChildElement(fov_node, "sensor_width")->getText());
				phys.sensor_height = getFloat(getChildElement(fov_node, "sensor_height")->getText());

				persp.fov = phys;
			}
			else
			{
				persp.fov = getFloat(fov_node.getText());
			}

			auto f_stop_it = getChildElement(camera_node, "f_stop");
			if (f_stop_it != camera_node.getChildren().end())
				persp.f_stop = getFloat(f_stop_it->getText());

			auto ap_blades_it = getChildElement(camera_node, "aperture_blades");
			if (ap_blades_it != camera_node.getChildren().end())
				persp.aperture_blades = getInt(ap_blades_it->getText());

			auto ap_rot_it = getChildElement(camera_node, "aperture_rotation");
			if (ap_rot_it != camera_node.getChildren().end())
				persp.aperture_rotation = getFloat(ap_rot_it->getText());

			auto shutter_it = getChildElement(camera_node, "shutter_speed");
			if (shutter_it != camera_node.getChildren().end())
				persp.shutter_speed = getFloat(shutter_it->getText());

			cam.projection = persp;
		}
		else if (projection == "orthographic")
		{
			Camera::Orthographic ortho = {};

			ortho.ortho_scale = getFloat(getChildElement(camera_node, "ortho_scale")->getText());

			cam.projection = ortho;
		}
		else if (projection == "fisheye")
		{
			Camera::Fisheye fisheye = {};

			fisheye.fisheye_fov = getFloat(getChildElement(camera_node, "fisheye_fov")->getText());

			const std::string& mapping_str = getChildElement(camera_node, "fisheye_mapping")->getText();
			if		(mapping_str == "equidistant")	fisheye.mapping = Camera::FisheyeMapping::EQUIDISTANT;
			else if	(mapping_str == "equisolid")	fisheye.mapping = Camera::FisheyeMapping::EQUISOLID;
			else if	(mapping_str == "orthographic")	fisheye.mapping = Camera::FisheyeMapping::ORTHOGRAPHIC;
			else									fisheye.mapping = Camera::FisheyeMapping::STEREIGRAPHIC;

			cam.projection = fisheye;
		}
		else if (projection == "panoramic")
		{
			Camera::Panoramic panoramic = {};

			const std::string& pan_type_str = getChildElement(camera_node, "panoramic_type")->getText();
			if (pan_type_str == "mercator") panoramic.panoramic_type = Camera::PanoramicType::MERCATOR;
			else panoramic.panoramic_type = Camera::PanoramicType::EQUIRECTANGULAR;

			cam.projection = panoramic;
		}

		const Node& placement = *getChildElement(camera_node, "placement");
		const std::string& placement_type = placement.getAttributes().find("type")->second.content;

		cam.position = getVec3(getChildElement(placement, "position")->getText());

		if (placement_type == "rotation")
		{
			const Node& rotation = *getChildElement(placement, "rotation");
			const std::string& rot_type = rotation.getAttributes().find("type")->second.content;

			if (rot_type == "quaternion")
				cam.rotation = getQuat(rotation.getText());
			else
			{
				auto angles = getVec3(rotation.getText());
				cam.rotation = quat::fromEuler(angles.x, angles.y, angles.z);
			}
		}
		else
		{
			Camera::LookAt lookat = {};

			lookat.lookat = getVec3(getChildElement(placement, "target")->getText());

			auto up_it = getChildElement(placement, "up");
			if (up_it != placement.getChildren().end())
				lookat.up = getVec3(up_it->getText());
			else
				lookat.up = vec3(0.0f, 1.0f, 0.0f);

			cam.rotation = lookat;
		}
	}
}

void SceneLoader::loadLights()
{
	auto it = getChildElement(ast_, "lights");

	if (it == ast_.getChildren().end()) return;

	const Node& lights = *it;

	for (const Node& light_node : lights.getChildren())
	{
		const auto& light_attr = light_node.getAttributes();

		Light light = {};

		auto label = light_attr.find("label");
		if (label != light_attr.end()) light.label = label->second.content;

		light.color = getColor(getChildElement(light_node, "color")->getText());

		auto intensity_it = getChildElement(light_node, "intensity");
		if (intensity_it != light_node.getChildren().end())
			light.intensity = getFloat(intensity_it->getText());

		const std::string& type = light_attr.find("type")->second.content;

		if (type == "point")
		{
			Light::Point point = {};
			point.position = getVec3(getChildElement(light_node, "position")->getText());
			light.projection = point;
		}
		else if (type == "directional")
		{
			Light::Directional directional = {};
			directional.direction = getVec3(getChildElement(light_node, "direction")->getText());
			light.projection = directional;
		}

		scene_.lights_.push_back(std::move(light));
	}
}

void SceneLoader::loadRender()
{
	auto it = getChildElement(ast_, "render");

	if (it == ast_.getChildren().end()) return;

	const Node& render = *it;

	RenderSettings render_settings;

	render_settings.width = getInt(getChildElement(render, "width")->getText());
	render_settings.height = getInt(getChildElement(render, "height")->getText());
	auto& cam = getChildElement(render, "camera")->getAttributes().find("ref")->second;
	render_settings.camera = cam.content;

	if (scene_.cameras_.find(cam.content) == scene_.cameras_.end())
	{
		errors_.report(TdrError(cam.content_line, cam.content_column, 1, "Render camera ref does not exist"));
		return ;
	}

	render_settings.max_bounces = getInt(getChildElement(render, "max_bounces")->getText());

	auto samples_it = getChildElement(render, "samples");
	if (samples_it != render.getChildren().end())
		render_settings.samples = getInt(samples_it->getText());

	auto output_it = getChildElement(render, "output");
	if (output_it != render.getChildren().end())
		render_settings.output_file = output_it->getText();
}

void SceneLoader::loadEnvironment()
{
	auto it = getChildElement(ast_, "environment");

	if (it == ast_.getChildren().end()) return;

	const Node& env = *it;

	const std::string& env_type = env.getAttributes().find("type")->second.content;

	if (env_type == "skybox")
	{
		Environment::Skybox skybox;

		skybox.path = getChildElement(env, "skybox")->getText();
		skybox.rotation = getFloat(getChildElement(env, "rotation")->getText());

		scene_.environment_.env = std::move(skybox);
	}
	else
	{
		scene_.environment_.env = getVec3(getChildElement(env, "color")->getText());
	}
}

Scene SceneLoader::load(const std::string& path)
{
	path_ = path;
	ParseResult res = SceneLanguageService::parse_file(path);
	std::vector<ParseResult> subfileRes;

	std::function<void(const Node&)> loadLinksRecursive = [&](const Node& ast) {
		for (const Node& link : ast.getChildren())
		{
			if (link.getIdentifier() == "link")
			{
				std::string subpath = link.getAttributes().find("path")->second.content;
				ParseResult subRes = SceneLanguageService::parse_file(subpath);
				loadLinksRecursive(subRes.ast);
				subfileRes.push_back(std::move(subRes));
			}
		}
	};
	loadLinksRecursive(res.ast);

	std::cout << "Link number : " << subfileRes.size() << std::endl;

	for (ParseResult& subRes : subfileRes) {
		for (const Node& subChild : subRes.ast.getChildren()) {
			if (subChild.getIdentifier() == "link")
				continue;
			bool foundSection = false;
			for (Node& mainChild : res.ast.children_) {
				if (mainChild.getIdentifier() == subChild.getIdentifier()) {
					foundSection = true;
					for (const Node& subItem : subChild.getChildren()) {
						bool isDuplicate = false;
						auto subNameIt = subItem.getAttributes().find("name");
						if (subNameIt != subItem.getAttributes().end()) {
							std::string subItemName = subNameIt->second.content;

							for (const Node& mainItem : mainChild.getChildren()) {
								auto mainNameIt = mainItem.getAttributes().find("name");
								if (mainNameIt != mainItem.getAttributes().end()) {
									if (mainNameIt->second.content == subItemName) {
										isDuplicate = true;
										std::cout << "  Skipping duplicate '" << subItemName << "' in section '" << mainChild.getIdentifier() << "'\n";
										break;
									}
								}
							}
						}
						if (!isDuplicate) {
							mainChild.children_.push_back(subItem);
						}
					}
					break;
				}
			}
			if (!foundSection) {
				res.ast.children_.push_back(subChild);
			}
		}
	}

	res.ast.children_.erase(
		std::remove_if(res.ast.children_.begin(), res.ast.children_.end(),
			[](const Node& n) { return n.getIdentifier() == "link"; }),
		res.ast.children_.end()
	);

	std::cout << "Merged scene created with all linked files\n";

	bool	has_error = false;

	for (TdrError e : res.errors)
	{
		if (e.getErrorLevel() == 1)
		{
			has_error = true;
			cu::logger::error(e.getError());
		}
		else if (e.getErrorLevel() == 2)
			cu::logger::warn(e.getError());
		else
			cu::logger::info(e.getError());
	}

	for (ParseResult& subres : subfileRes) {
		for (TdrError e : subres.errors)
		{
			if (e.getErrorLevel() == 1)
			{
				has_error = true;
				cu::logger::error(e.getError());
			}
			else if (e.getErrorLevel() == 2)
				cu::logger::warn(e.getError());
			else
				cu::logger::info(e.getError());
		}
	}

	if (has_error) throw std::runtime_error("Cannot open the scene with an error present on the file.");

	ast_ = std::move(res.ast);

	std::cout << "\n=== FINAL MERGED AST ===\n" << std::endl;
	ast_.print();
	std::cout << "\n========================\n" << std::endl;

	loadTextures();
	loadMaterials();
	loadAssets();
	loadCameras();
	loadLights();

	for (TdrError e : errors_.get_errors())
	{
		if (e.location.filepath.empty()) e.location.filepath = path;
		if (e.getErrorLevel() == 1)
		{
			has_error = true;
			cu::logger::error(e.getError());
		}
		else if (e.getErrorLevel() == 2)
			cu::logger::warn(e.getError());
		else
			cu::logger::info(e.getError());
	}

	if (has_error)
		throw std::runtime_error("Cannot open the scene with an error present on the file.");
	return std::move(scene_);
}

}
