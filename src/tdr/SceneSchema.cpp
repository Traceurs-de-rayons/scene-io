#include "tdr/SceneSchema.hpp"
#include <iostream>
namespace sceneIO::tdr {

const std::string printValueType(ValueType type)
{
	switch (type)
	{
		case ValueType::STRING:		return "string";
		case ValueType::FLOAT:		return "float";
		case ValueType::INT:		return "integer";
		case ValueType::BOOL:		return "boolean";
		case ValueType::VEC3:		return "vec3";
		case ValueType::COLOR:		return "color";
		case ValueType::FILEPATH:	return "filepath";
		case ValueType::ENUM:		return "enum";
		default: break;
	}
	return "";
}

void SceneSchema::build_schema()
{
	root = TagSchema{
		.name = "root",
		.required = true,
		.allow_text = false,
		.hover_info = "",
		.completion_detail = "",
		.examples = {}
	};

	root.children["link"] = TagSchema{
		.name = "link",
		.required = false,
		.allow_text = false,
		.hover_info = "Link another scene file. It allow you to split scene content across multiple files.",
		.completion_detail = "Link another scene file",
		.allow_multiple = false
	};

	root.children["link"].attributes["path"] = AttributeSchema{
		.name = "path",
		.required = true,
		.type = ValueType::FILEPATH,
		.hover_info = "The path of the linked file.",
		.completion_detail = "File path",
		.examples = {"path=\"./material.tdr\""}
	};

	root.children["materials"] = TagSchema{
		.name = "materials",
		.required = false,
		.allow_text = false,
		.hover_info = "List of materials in the scene. Contains <material> tags.",
		.completion_detail = "Materials container",
		.allow_multiple = false
	};

	root.children["materials"].children["material"] = TagSchema{
		.name = "material",
		.required = false,
		.allow_text = false,
		.hover_info = "Material definition with all its properties.",
		.completion_detail = "Material definition",
		.allow_multiple = true
	};

	root.children["materials"].children["material"].attributes["name"] = AttributeSchema{
		.name = "name",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "Unique identifier for this material. Used to reference it in objects.",
		.completion_detail = "Material name (required)",
		.examples = {"name=\"metal\"", "name=\"wood\""}
	};

	root.children["materials"].children["material"].children["color"] = TagSchema{
		.name = "color",
		.required = true,
		.allow_text = true,
		.text_type = ValueType::COLOR,
		.hover_info = "RGB color of the material. Format: \"r,g,b\" or \"#RRGGBB\"",
		.completion_detail = "Material color (required)",
		.examples = {"<color>255,0,0</color>", "<color>#FF0000</color>"}
	};

	root.children["materials"].children["material"].children["ior"] = TagSchema{
		.name = "ior",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::FLOAT,
		.hover_info = "IOR of the material.",
		.completion_detail = "Material ior (required)",
		.examples = {"<ior>1.5</ior>", "<ior>0.47</ior>"}
	};

	root.children["materials"].children["material"].children["roughness"] = TagSchema{
		.name = "roughness",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::FLOAT,
		.range = std::make_pair(0.0, 1.0),
		.hover_info = "Roughness of the material.",
		.completion_detail = "Material roughness (required)",
		.examples = {"<roughness>0.0</roughness>", "<roughness>0.5</roughness>"}
	};
	
	root.children["camera"] = TagSchema{
		.name = "camera",
		.required = true,
		.allow_text = false,
		.hover_info = "Camera definition with position, rotation and field of view.",
		.completion_detail = "Camera",
		.allow_multiple = false
	};
	
	root.children["camera"].attributes["position"] = AttributeSchema{
		.name = "position",
		.required = true,
		.type = ValueType::VEC3,
		.hover_info = "3D position of the camera in world space (x,y,z).",
		.completion_detail = "Camera position (required)",
		.examples = {"position=\"0,42,0\""}
	};
	
	root.children["camera"].attributes["fov"] = AttributeSchema{
		.name = "fov",
		.required = false,
		.type = ValueType::FLOAT,
		.default_value = "60",
		.range = {{1.0f, 180.0f}},
		.hover_info = "Field of view in degrees. Default: 60. Range: [1, 180]",
		.completion_detail = "Field of view (optional, default: 60)",
		.examples = {"fov=\"90\"", "fov=\"45.5\""}
	};
}

const TagSchema *SceneSchema::findTagRecursive(const TagSchema& tag, const std::string& tagName) const
{
	if (tag.name == tagName)
		return &tag;
	else
	{
		for (auto it = tag.children.begin(); it != tag.children.end(); it++)
		{
			auto res = findTagRecursive(it->second, tagName);
			if (res) return res;
		}
	}
	return nullptr;
}

const TagSchema *SceneSchema::getTagSchema(const std::string& tagName) const
{
	return findTagRecursive(root, tagName);
}

const AttributeSchema *SceneSchema::getAttributeSchema(const std::string& tagName, const std::string& attrName) const
{
	const TagSchema *tag = getTagSchema(tagName);

	if (!tag) return nullptr;

	for (auto it = tag->attributes.begin(); it != tag->attributes.end(); it++)
		if (it->first == attrName) return &it->second;

	return nullptr;
}

}
