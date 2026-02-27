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

void TagSchema::include(const std::map<std::string, TagSchema>& group)
{
	for (const auto& [name, tag] : group)
		children[name] = tag;
}

void ConditionalVariant::include(const std::map<std::string, TagSchema>& group)
{
	for (const auto& [name, tag] : group)
		children[name] = tag;
}

const ConditionalVariant* TagSchema::getMatchingVariant(const std::string& discriminator_value) const
{
	for (const auto& variant : variants)
	{
		if (variant.discriminator_value == discriminator_value)
			return &variant;
	}
	return nullptr;
}

void SceneSchema::build_tag_groups()
{
	std::map<std::string, TagSchema> transform_group;

	transform_group["transform"] = TagSchema{
		.name = "transform",
		.required = false,
		.allow_text = false,
		.hover_info = "Transformation applied to the element (position, rotation, scale).",
		.completion_detail = "Transform",
		.allow_multiple = false
	};

	transform_group["transform"].children["position"] = TagSchema{
		.name = "position",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::VEC3,
		.hover_info = "3D position in world space. Format: \"x y z\"",
		.completion_detail = "Position (vec3)",
		.examples = {"<position>0 0 0</position>", "<position>3.5 1 -2</position>"},
		.allow_multiple = false
	};

	transform_group["transform"].children["rotation"] = TagSchema{
		.name = "rotation",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::VEC3,
		.hover_info = "Rotation. Format depends on type attribute.",
		.completion_detail = "Rotation (vec3)",
		.examples = {"<rotation type=\"euler\" order=\"xyz\">0 45 0</rotation>"},
		.allow_multiple = false
	};

	transform_group["transform"].children["rotation"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "euler",
		.enum_values = {"euler", "quaternion"},
		.hover_info = "Rotation type: euler or quaternion.",
		.completion_detail = "Rotation type"
	};

	transform_group["transform"].children["rotation"].attributes["order"] = AttributeSchema{
		.name = "order",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "xyz",
		.enum_values = {"xyz", "xzy", "yxz", "yzx", "zxy", "zyx"},
		.hover_info = "Euler rotation order.",
		.completion_detail = "Euler order"
	};

	transform_group["transform"].children["scale"] = TagSchema{
		.name = "scale",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::VEC3,
		.hover_info = "Scale factor. Format: \"x y z\"",
		.completion_detail = "Scale (vec3)",
		.examples = {"<scale>1 1 1</scale>", "<scale>0.5 0.5 0.5</scale>"},
		.allow_multiple = false
	};

	tag_groups["transform"] = std::move(transform_group);
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

	root.children["camera"].include(tag_groups["transform"]);

	root.children["assets"] = TagSchema{
		.name = "assets",
		.required = false,
		.allow_text = false,
		.hover_info = "List of assets in the scene.",
		.completion_detail = "Assets container",
		.allow_multiple = false
	};

	auto& asset = root.children["assets"].children["asset"];
	asset = TagSchema{
		.name = "asset",
		.required = false,
		.allow_text = false,
		.hover_info = "Asset definition. Type determines available children.",
		.completion_detail = "Asset definition",
		.allow_multiple = true
	};

	asset.attributes["type"] = AttributeSchema{
		.name = "type",
		.required = true,
		.type = ValueType::ENUM,
		.enum_values = {"object", "primitive", "instance"},
		.hover_info = "Type of asset: object (mesh from file), primitive (built-in shape), or instance (reference to another asset).",
		.completion_detail = "Asset type (required)"
	};

	asset.attributes["id"] = AttributeSchema{
		.name = "id",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "Unique identifier for this asset.",
		.completion_detail = "Asset ID (required)",
		.examples = {"id=\"my_object\""}
	};

	{
		ConditionalVariant v;
		v.discriminator_attr = "type";
		v.discriminator_value = "object";
		v.hover_info = "Object asset: loads a mesh from a file.";

		v.children["object"] = TagSchema{
			.name = "object",
			.required = true,
			.allow_text = false,
			.hover_info = "Object mesh source.",
			.completion_detail = "Object source",
			.allow_multiple = false
		};
		v.children["object"].attributes["path"] = AttributeSchema{
			.name = "path",
			.required = true,
			.type = ValueType::FILEPATH,
			.hover_info = "Path to the mesh file.",
			.completion_detail = "Mesh file path",
			.examples = {"path=\"monobjet.obj\""}
		};

		v.children["material"] = TagSchema{
			.name = "material",
			.required = false,
			.allow_text = false,
			.hover_info = "Material reference.",
			.completion_detail = "Material ref",
			.allow_multiple = false
		};
		v.children["material"].attributes["ref"] = AttributeSchema{
			.name = "ref",
			.required = true,
			.type = ValueType::STRING,
			.hover_info = "Reference to a material name.",
			.completion_detail = "Material reference"
		};

		v.include(tag_groups["transform"]);

		asset.variants.push_back(std::move(v));
	}

	{
		ConditionalVariant v;
		v.discriminator_attr = "type";
		v.discriminator_value = "primitive";
		v.hover_info = "Primitive asset: a built-in shape.";

		v.children["primitive"] = TagSchema{
			.name = "primitive",
			.required = true,
			.allow_text = false,
			.hover_info = "Primitive shape type.",
			.completion_detail = "Primitive type",
			.allow_multiple = false
		};
		v.children["primitive"].attributes["type"] = AttributeSchema{
			.name = "type",
			.required = true,
			.type = ValueType::ENUM,
			.enum_values = {"plane", "sphere", "cube", "cylinder", "cone"},
			.hover_info = "Shape type.",
			.completion_detail = "Shape type (required)"
		};

		v.children["material"] = TagSchema{
			.name = "material",
			.required = false,
			.allow_text = false,
			.hover_info = "Material reference.",
			.completion_detail = "Material ref",
			.allow_multiple = false
		};
		v.children["material"].attributes["ref"] = AttributeSchema{
			.name = "ref",
			.required = true,
			.type = ValueType::STRING,
			.hover_info = "Reference to a material name.",
			.completion_detail = "Material reference"
		};

		v.include(tag_groups["transform"]);

		asset.variants.push_back(std::move(v));
	}

	{
		ConditionalVariant v;
		v.discriminator_attr = "type";
		v.discriminator_value = "instance";
		v.hover_info = "Instance asset: reference to another asset with a different transform.";

		v.attributes["parent"] = AttributeSchema{
			.name = "parent",
			.required = true,
			.type = ValueType::STRING,
			.hover_info = "ID of the parent asset to instance.",
			.completion_detail = "Parent asset ID (required)",
			.examples = {"parent=\"truc\""}
		};

		v.include(tag_groups["transform"]);

		asset.variants.push_back(std::move(v));
	}
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
