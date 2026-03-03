#include "tdr/SceneSchema.hpp"
#include <math.h>

namespace sceneIO::tdr {

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

	{
		ConditionalVariant v0;
		v0.discriminator_attr = "type";
		v0.discriminator_value = "euler";
		v0.hover_info = "Euler rotation (yaw, pitch, roll)";
		v0.allow_text = true;
		v0.text_type = ValueType::VEC3;
		v0.range = std::make_pair(0, 360);

		transform_group["transform"].children["rotation"].variants.push_back(std::move(v0));
	}

	{
		ConditionalVariant v1;
		v1.discriminator_attr = "type";
		v1.discriminator_value = "quaternion";
		v1.hover_info = "Surface rotation, expressed as a unit quaternion (x y z w).\n\nThe quaternion must be normalized : x² + y² + z² + w² = 1.\nUnnormalized values will be rejected by the parser.";
		v1.allow_text = true;

		transform_group["transform"].children["rotation"].variants.push_back(std::move(v1));
	}

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
		.completion_detail = "Material name",
		.examples = {"name=\"metal\"", "name=\"wood\""}
	};

	root.children["materials"].children["material"].attributes["label"] = AttributeSchema{
		.name = "label",
		.required = false,
		.type = ValueType::STRING,
		.hover_info = "Display name of the material",
		.completion_detail = "Display name"
	};

	root.children["materials"].children["material"].children["albedo"] = TagSchema{
		.name = "albedo",
		.required = false,
		.allow_text = false,
		.hover_info = "Base surface color that defines the proportion of incoming light reflected diffusely. It does not include lighting, shadows, reflections, or emission — only the intrinsic reflectance of the material.",
		.completion_detail = "Material color",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["albedo"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "rgb",
		.enum_values = {"rgb", "texture"},
		.hover_info = "",
		.completion_detail = "Albedo type"
	};

	{
		ConditionalVariant v2;
		v2.discriminator_attr = "type";
		v2.discriminator_value = "rgb";
		v2.allow_text = true;
		v2.text_type = ValueType::COLOR;

		root.children["materials"].children["material"].children["albedo"].variants.push_back(std::move(v2));
	}

	{
		ConditionalVariant v3;
		v3.discriminator_attr = "type";
		v3.discriminator_value = "texture";
		v3.allow_text = true;

		root.children["materials"].children["material"].children["albedo"].variants.push_back(std::move(v3));
	}

	root.children["materials"].children["material"].children["metallic"] = TagSchema{
		.name = "metallic",
		.required = false,
		.allow_text = false,
		.hover_info = "Whether the surface behaves as a conductor (1.0) or a dielectric (0.0). Metals have no diffuse component and their reflectance is tinted by the albedo. Intermediate values are physically incorrect but accepted as an artistic control.",
		.completion_detail = "Material metallic",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["metallic"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "value",
		.enum_values = {"value", "texture"},
		.hover_info = "",
		.completion_detail = "Metallic type"
	};

	{
		ConditionalVariant v4;
		v4.discriminator_attr = "type";
		v4.discriminator_value = "value";
		v4.allow_text = true;
		v4.text_type = ValueType::FLOAT;
		v4.range = std::make_pair(0, 1);

		root.children["materials"].children["material"].children["metallic"].variants.push_back(std::move(v4));
	}

	{
		ConditionalVariant v5;
		v5.discriminator_attr = "type";
		v5.discriminator_value = "texture";
		v5.allow_text = true;

		root.children["materials"].children["material"].children["metallic"].variants.push_back(std::move(v5));
	}

	root.children["materials"].children["material"].children["roughness"] = TagSchema{
		.name = "roughness",
		.required = false,
		.allow_text = false,
		.hover_info = "Microscale irregularity of the surface. 0.0 is a perfect mirror, 1.0 is fully diffuse. Drives the width of the specular lobe via the GGX distribution.",
		.completion_detail = "Material roughness",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["roughness"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "value",
		.enum_values = {"value", "texture"},
		.hover_info = "",
		.completion_detail = "Roughness type"
	};

	{
		ConditionalVariant v6;
		v6.discriminator_attr = "type";
		v6.discriminator_value = "value";
		v6.allow_text = true;
		v6.text_type = ValueType::FLOAT;
		v6.range = std::make_pair(0, 1);

		root.children["materials"].children["material"].children["roughness"].variants.push_back(std::move(v6));
	}

	{
		ConditionalVariant v7;
		v7.discriminator_attr = "type";
		v7.discriminator_value = "texture";
		v7.allow_text = true;

		root.children["materials"].children["material"].children["roughness"].variants.push_back(std::move(v7));
	}

	root.children["materials"].children["material"].children["transmission"] = TagSchema{
		.name = "transmission",
		.required = false,
		.allow_text = false,
		.hover_info = "Fraction of light that passes through the surface rather than being reflected. 1.0 is fully transparent. Requires a valid IOR to refract correctly.",
		.completion_detail = "Material transmission",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["transmission"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "value",
		.enum_values = {"value", "texture"},
		.hover_info = "",
		.completion_detail = "Transmission type"
	};

	{
		ConditionalVariant v8;
		v8.discriminator_attr = "type";
		v8.discriminator_value = "value";
		v8.allow_text = true;
		v8.text_type = ValueType::FLOAT;
		v8.range = std::make_pair(0, 1);

		root.children["materials"].children["material"].children["transmission"].variants.push_back(std::move(v8));
	}

	{
		ConditionalVariant v9;
		v9.discriminator_attr = "type";
		v9.discriminator_value = "texture";
		v9.allow_text = true;

		root.children["materials"].children["material"].children["transmission"].variants.push_back(std::move(v9));
	}

	root.children["materials"].children["material"].children["ambient_occlusion"] = TagSchema{
		.name = "ambient_occlusion",
		.required = false,
		.allow_text = false,
		.hover_info = "Multiplier that darkens crevices by approximating how much ambient light reaches a point. Not physically based — It allows you to add texture to a surface that doesn't really have any. Useful as a baked artistic control.",
		.completion_detail = "Material metallic",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["ambient_occlusion"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "value",
		.enum_values = {"value", "texture"},
		.hover_info = "",
		.completion_detail = "Ambiant occlusion type"
	};

	{
		ConditionalVariant v10;
		v10.discriminator_attr = "type";
		v10.discriminator_value = "value";
		v10.allow_text = true;
		v10.text_type = ValueType::FLOAT;
		v10.range = std::make_pair(0, 1);

		root.children["materials"].children["material"].children["ambient_occlusion"].variants.push_back(std::move(v10));
	}

	{
		ConditionalVariant v11;
		v11.discriminator_attr = "type";
		v11.discriminator_value = "texture";
		v11.allow_text = true;

		root.children["materials"].children["material"].children["ambient_occlusion"].variants.push_back(std::move(v11));
	}

	root.children["materials"].children["material"].children["emission_strength"] = TagSchema{
		.name = "emission_strength",
		.required = false,
		.allow_text = false,
		.hover_info = "Power of the light emitted by the surface. A value above 0 turns the surface into an area light that contributes to global illumination.",
		.completion_detail = "Material emission strength",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["emission_strength"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "value",
		.enum_values = {"value", "texture"},
		.hover_info = "",
		.completion_detail = "Emission strength type"
	};

	{
		ConditionalVariant v12;
		v12.discriminator_attr = "type";
		v12.discriminator_value = "value";
		v12.allow_text = true;
		v12.text_type = ValueType::FLOAT;

		root.children["materials"].children["material"].children["emission_strength"].variants.push_back(std::move(v12));
	}

	{
		ConditionalVariant v13;
		v13.discriminator_attr = "type";
		v13.discriminator_value = "texture";
		v13.allow_text = true;

		root.children["materials"].children["material"].children["emission_strength"].variants.push_back(std::move(v13));
	}

	root.children["materials"].children["material"].children["emission_color"] = TagSchema{
		.name = "emission_color",
		.required = false,
		.allow_text = false,
		.hover_info = "Color of the emitted light. Multiplied by emission_strength to give the final radiance.",
		.completion_detail = "Material emission color",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["emission_color"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "rgb",
		.enum_values = {"rgb", "texture"},
		.hover_info = "",
		.completion_detail = "Emission color type"
	};

	{
		ConditionalVariant v14;
		v14.discriminator_attr = "type";
		v14.discriminator_value = "rgb";
		v14.allow_text = true;
		v14.text_type = ValueType::COLOR;

		root.children["materials"].children["material"].children["emission_color"].variants.push_back(std::move(v14));
	}

	{
		ConditionalVariant v15;
		v15.discriminator_attr = "type";
		v15.discriminator_value = "texture";
		v15.allow_text = true;

		root.children["materials"].children["material"].children["emission_color"].variants.push_back(std::move(v15));
	}

	root.children["materials"].children["material"].children["ior"] = TagSchema{
		.name = "ior",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::FLOAT,
		.hover_info = "Index of Refraction. Controls how much light bends when entering the surface and the fresnel reflectance at grazing angles. 1.0 is vacuum, common values are 1.33 (water), 1.5 (glass), 2.4 (diamond). Only meaningful for dielectrics.",
		.completion_detail = "Material IOR",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["texture_scale"] = TagSchema{
		.name = "texture_scale",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::FLOAT,
		.hover_info = "Uniform scale applied to UV coordinates before sampling any texture. Values above 1.0 tile the texture, below 1.0 zoom in.",
		.completion_detail = "Scale of the material textures",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["normal_map"] = TagSchema{
		.name = "normal_map",
		.required = false,
		.allow_text = true,
		.hover_info = "Tangent-space normal map. RGB values are interpreted as XYZ perturbations of the surface normal, remapped from [0,1] to [-1,1].",
		.completion_detail = "Texture normal perbations",
		.allow_multiple = false
	};

	root.children["materials"].children["material"].children["normal_intensity"] = TagSchema{
		.name = "normal_intensity",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::FLOAT,
		.hover_info = "Blending factor between the geometry normal (0.0) and the fully perturbed normal from the normal map (1.0). Behavior with values outside the range [0-1] is undefined.",
		.completion_detail = "",
		.allow_multiple = false
	};

	root.children["assets"] = TagSchema{
		.name = "assets",
		.required = false,
		.allow_text = false,
		.hover_info = "List of assets in the scene.",
		.completion_detail = "Assets container",
		.allow_multiple = false
	};

	root.children["assets"].children["asset"] = TagSchema{
		.name = "asset",
		.required = false,
		.allow_text = false,
		.hover_info = "Asset definition. Type determines available children.",
		.completion_detail = "Asset definition",
		.allow_multiple = true
	};

	root.children["assets"].children["asset"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = true,
		.type = ValueType::ENUM,
		.enum_values = {"object", "primitive", "instance"},
		.hover_info = "Type of asset: object (mesh from file), primitive (built-in shape), or instance (reference to another asset).",
		.completion_detail = "Asset type (required)"
	};

	root.children["assets"].children["asset"].attributes["id"] = AttributeSchema{
		.name = "id",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "Unique identifier for this asset.",
		.completion_detail = "Asset ID (required)",
		.examples = {"id=\"my_object\""}
	};

	root.children["assets"].children["asset"].children["material"] = TagSchema{
		.name = "material",
		.required = false,
		.allow_text = false,
		.hover_info = "Reference to the material of the asset",
		.completion_detail = "Asset material",
		.allow_multiple = false
	};

	root.children["assets"].children["asset"].children["material"].attributes["ref"] = AttributeSchema{
		.name = "ref",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "Name of the material",
		.completion_detail = "Name of the material"
	};

	root.children["assets"].children["asset"].include(tag_groups["transform"]);

	{
		ConditionalVariant v16;
		v16.discriminator_attr = "type";
		v16.discriminator_value = "object";
		v16.hover_info = "Object asset: loads a mesh from a file.";
		v16.allow_text = false;

		v16.children["object"] = TagSchema{
			.name = "object",
			.required = true,
			.allow_text = false,
			.hover_info = "Object mesh source.",
			.completion_detail = "Object source",
			.allow_multiple = false
		};

		v16.children["object"].attributes["type"] = AttributeSchema{
			.name = "type",
			.required = false,
			.type = ValueType::ENUM,
			.default_value = "external",
			.enum_values = {"external", "raw"},
			.hover_info = "Type of the imported object",
			.completion_detail = "Type of the imported object"
		};

		{
			ConditionalVariant v17;
			v17.discriminator_attr = "type";
			v17.discriminator_value = "raw";
			v17.hover_info = "Raw .obj formated mesh.";
			v17.allow_text = true;

			v16.children["object"].variants.push_back(std::move(v17));
		}

		{
			ConditionalVariant v18;
			v18.discriminator_attr = "type";
			v18.discriminator_value = "external";
			v18.hover_info = "External file object mesh source.";
			v18.allow_text = false;

			v18.attributes["path"] = AttributeSchema{
				.name = "path",
				.required = true,
				.type = ValueType::FILEPATH,
				.hover_info = "Filepath of the object.",
				.completion_detail = "Filepath of the object"
			};

			v16.children["object"].variants.push_back(std::move(v18));
		}

		root.children["assets"].children["asset"].variants.push_back(std::move(v16));
	}

	{
		ConditionalVariant v19;
		v19.discriminator_attr = "type";
		v19.discriminator_value = "primitive";
		v19.hover_info = "Primitive asset: a built-in shape.";
		v19.allow_text = false;

		v19.children["primitive"] = TagSchema{
			.name = "primitive",
			.required = true,
			.allow_text = false,
			.hover_info = "Primitive shape type.",
			.completion_detail = "Primitive type",
			.allow_multiple = false
		};

		v19.children["primitive"].attributes["type"] = AttributeSchema{
			.name = "type",
			.required = true,
			.type = ValueType::ENUM,
			.enum_values = {"plane", "sphere", "cylinder", "cone", "hyperboloid"},
			.hover_info = "Shape type.",
			.completion_detail = "Shape type (required)"
		};

		{
			ConditionalVariant v20;
			v20.discriminator_attr = "type";
			v20.discriminator_value = "plane";
			v20.hover_info = "Plane primitive.";
			v20.allow_text = false;

			v20.children["normal"] = TagSchema{
				.name = "normal",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::VEC3,
				.range = std::make_pair(-1, 1),
				.hover_info = "Define the orientation of the plane",
				.completion_detail = "Plane normal",
				.allow_multiple = false
			};

			v19.children["primitive"].variants.push_back(std::move(v20));
		}

		{
			ConditionalVariant v21;
			v21.discriminator_attr = "type";
			v21.discriminator_value = "sphere";
			v21.hover_info = "Sphere primitive.";
			v21.allow_text = false;

			v21.children["radius"] = TagSchema{
				.name = "radius",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Radius of the sphere",
				.completion_detail = "Sphere radius",
				.allow_multiple = false
			};

			v19.children["primitive"].variants.push_back(std::move(v21));
		}

		{
			ConditionalVariant v22;
			v22.discriminator_attr = "type";
			v22.discriminator_value = "cylinder";
			v22.hover_info = "Cylinder primitive.";
			v22.allow_text = false;

			v22.children["radius"] = TagSchema{
				.name = "radius",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Radius of the cylinder",
				.completion_detail = "Cylinder radius",
				.allow_multiple = false
			};

			v22.children["height"] = TagSchema{
				.name = "height",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Height of the cylinder",
				.completion_detail = "Cylinder height",
				.allow_multiple = false
			};

			v19.children["primitive"].variants.push_back(std::move(v22));
		}

		{
			ConditionalVariant v23;
			v23.discriminator_attr = "type";
			v23.discriminator_value = "cone";
			v23.hover_info = "Cone primitive.";
			v23.allow_text = false;

			v23.children["radius"] = TagSchema{
				.name = "radius",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Radius of the cone",
				.completion_detail = "Cone radius",
				.allow_multiple = false
			};

			v23.children["height"] = TagSchema{
				.name = "height",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Height of the cone",
				.completion_detail = "Cone height",
				.allow_multiple = false
			};

			v19.children["primitive"].variants.push_back(std::move(v23));
		}

		{
			ConditionalVariant v24;
			v24.discriminator_attr = "type";
			v24.discriminator_value = "hyperboloid";
			v24.hover_info = "Hyperboloid primitive.";
			v24.allow_text = false;

			v24.children["height"] = TagSchema{
				.name = "height",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Height of the hyperboloid",
				.completion_detail = "Hyperboloid height",
				.allow_multiple = false
			};

			v24.children["a"] = TagSchema{
				.name = "a",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "",
				.completion_detail = "",
				.allow_multiple = false
			};

			v24.children["b"] = TagSchema{
				.name = "b",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "",
				.completion_detail = "",
				.allow_multiple = false
			};

			v24.children["c"] = TagSchema{
				.name = "c",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "",
				.completion_detail = "",
				.allow_multiple = false
			};

			v24.children["shape"] = TagSchema{
				.name = "shape",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Shape of the hyperboloid",
				.completion_detail = "Hyperboloid shape",
				.allow_multiple = false
			};

			v19.children["primitive"].variants.push_back(std::move(v24));
		}

		root.children["assets"].children["asset"].variants.push_back(std::move(v19));
	}

	{
		ConditionalVariant v25;
		v25.discriminator_attr = "type";
		v25.discriminator_value = "instance";
		v25.hover_info = "Instance asset: reference to another asset with a different transform.";
		v25.allow_text = false;

		v25.children["parent"] = TagSchema{
			.name = "parent",
			.required = true,
			.allow_text = false,
			.hover_info = "Name of the parent asset",
			.completion_detail = "Parent asset name",
			.allow_multiple = false
		};

		v25.children["parent"].attributes["ref"] = AttributeSchema{
			.name = "ref",
			.required = true,
			.type = ValueType::STRING,
			.hover_info = "Name of the parent asset",
			.completion_detail = "Parent asset name"
		};

		root.children["assets"].children["asset"].variants.push_back(std::move(v25));
	}

	root.children["cameras"] = TagSchema{
		.name = "cameras",
		.required = false,
		.allow_text = false,
		.hover_info = "Contain a list of cameras.",
		.completion_detail = "Cameras",
		.allow_multiple = false
	};

	root.children["cameras"].children["camera"] = TagSchema{
		.name = "camera",
		.required = false,
		.allow_text = false,
		.hover_info = "A camera and its properties.",
		.completion_detail = "Camera",
		.allow_multiple = true
	};

	root.children["cameras"].children["camera"].attributes["projection"] = AttributeSchema{
		.name = "projection",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "perspective",
		.enum_values = {"perspective", "orthographic", "panoramic", "fisheye"},
		.hover_info = "Projection of the camera",
		.completion_detail = "Projection of the camera"
	};

	root.children["cameras"].children["camera"].attributes["name"] = AttributeSchema{
		.name = "name",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "Camera unique identifier, used to set the render camera",
		.completion_detail = "Camera unique identifier"
	};

	root.children["cameras"].children["camera"].attributes["label"] = AttributeSchema{
		.name = "label",
		.required = false,
		.type = ValueType::STRING,
		.hover_info = "Display name of the camera",
		.completion_detail = "Display name"
	};

	root.children["cameras"].children["camera"].children["placement"] = TagSchema{
		.name = "placement",
		.required = true,
		.allow_text = false,
		.hover_info = "Camera placement settings",
		.completion_detail = "Camera placement settings",
		.allow_multiple = false
	};

	root.children["cameras"].children["camera"].children["placement"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "lookat",
		.enum_values = {"lookat", "rotation"},
		.hover_info = "Type of space position",
		.completion_detail = "Type of space position"
	};

	root.children["cameras"].children["camera"].children["placement"].children["position"] = TagSchema{
		.name = "position",
		.required = true,
		.allow_text = true,
		.text_type = ValueType::VEC3,
		.hover_info = "Camera position in the world",
		.completion_detail = "Camera position",
		.allow_multiple = false
	};

	{
		ConditionalVariant v26;
		v26.discriminator_attr = "type";
		v26.discriminator_value = "lookat";
		v26.hover_info = "Easy camera placement defined with a lookat";
		v26.allow_text = false;

		v26.children["target"] = TagSchema{
			.name = "target",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::VEC3,
			.hover_info = "Camera target/look at. Represent a point of the world where the camera is looking.",
			.completion_detail = "Camera target",
			.allow_multiple = false
		};

		v26.children["up"] = TagSchema{
			.name = "up",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::VEC3,
			.range = std::make_pair(-1, 1),
			.hover_info = "Camera up vector. Used to set a roll to the camera.",
			.completion_detail = "Camera up vector",
			.allow_multiple = false
		};

		root.children["cameras"].children["camera"].children["placement"].variants.push_back(std::move(v26));
	}

	{
		ConditionalVariant v27;
		v27.discriminator_attr = "type";
		v27.discriminator_value = "rotation";
		v27.hover_info = "Camera placement defined with a rotation";
		v27.allow_text = false;

		v27.children["rotation"] = TagSchema{
			.name = "rotation",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::VEC3,
			.hover_info = "Rotation. Format depends on type attribute.",
			.completion_detail = "Rotation (vec3)",
			.allow_multiple = false
		};

		v27.children["rotation"].attributes["type"] = AttributeSchema{
			.name = "type",
			.required = false,
			.type = ValueType::ENUM,
			.default_value = "euler",
			.enum_values = {"euler", "quaternion"},
			.hover_info = "Rotation type: euler or quaternion.",
			.completion_detail = "Rotation type"
		};

		{
			ConditionalVariant v28;
			v28.discriminator_attr = "type";
			v28.discriminator_value = "euler";
			v28.hover_info = "Euler rotation (yaw, pitch, roll)";
			v28.allow_text = true;
			v28.text_type = ValueType::VEC3;
			v28.range = std::make_pair(0, 360);

			v27.children["rotation"].variants.push_back(std::move(v28));
		}

		{
			ConditionalVariant v29;
			v29.discriminator_attr = "type";
			v29.discriminator_value = "quaternion";
			v29.hover_info = "Surface rotation, expressed as a unit quaternion (x y z w).\n\nThe quaternion must be normalized : x² + y² + z² + w² = 1.\nUnnormalized values will be rejected by the parser.";
			v29.allow_text = true;

			v27.children["rotation"].variants.push_back(std::move(v29));
		}

		root.children["cameras"].children["camera"].children["placement"].variants.push_back(std::move(v27));
	}

	{
		ConditionalVariant v30;
		v30.discriminator_attr = "projection";
		v30.discriminator_value = "perspective";
		v30.hover_info = "A perspective camera and its properties.";
		v30.allow_text = false;

		v30.children["fov"] = TagSchema{
			.name = "fov",
			.required = true,
			.allow_text = false,
			.hover_info = "Field of view of the camera",
			.completion_detail = "FOV of the camera",
			.allow_multiple = false
		};

		v30.children["fov"].attributes["mode"] = AttributeSchema{
			.name = "mode",
			.required = false,
			.type = ValueType::ENUM,
			.default_value = "simple",
			.enum_values = {"simple", "physical"},
			.hover_info = "Configuration mode of the FOV",
			.completion_detail = "Configuration mode of the FOV"
		};

		{
			ConditionalVariant v31;
			v31.discriminator_attr = "mode";
			v31.discriminator_value = "simple";
			v31.hover_info = "Simple setup of the FOV";
			v31.allow_text = true;
			v31.text_type = ValueType::FLOAT;
			v31.range = std::make_pair(1, 180);

			v30.children["fov"].variants.push_back(std::move(v31));
		}

		{
			ConditionalVariant v32;
			v32.discriminator_attr = "mode";
			v32.discriminator_value = "physical";
			v32.hover_info = "Physical setup of the FOV";
			v32.allow_text = false;

			v32.attributes["sensor_fit"] = AttributeSchema{
				.name = "sensor_fit",
				.required = false,
				.type = ValueType::ENUM,
				.default_value = "horizontal",
				.enum_values = {"horizontal", "vertical"},
				.hover_info = "Direction of the sensor used. If the sensor fit is horizontal, the sensor height will be adapted to match the resolution of the render. And that's the opposite with a vertical sensor fit.",
				.completion_detail = "Direction of the sensor used"
			};

			v32.children["focal_length"] = TagSchema{
				.name = "focal_length",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Focal length of the camera",
				.completion_detail = "Focal length",
				.allow_multiple = false
			};

			v32.children["sensor_width"] = TagSchema{
				.name = "sensor_width",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Width of the sensor of your camera",
				.completion_detail = "Sensor width",
				.allow_multiple = false
			};

			v32.children["sensor_height"] = TagSchema{
				.name = "sensor_height",
				.required = true,
				.allow_text = true,
				.text_type = ValueType::FLOAT,
				.hover_info = "Height of the sensor of your camera",
				.completion_detail = "Sensor Height",
				.allow_multiple = false
			};

			v30.children["fov"].variants.push_back(std::move(v32));
		}

		v30.children["f_stop"] = TagSchema{
			.name = "f_stop",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.range = std::make_pair(0.5, 64),
			.hover_info = "Aperture of the lens, expressed as an f-stop (f/N).\n\nControls the radius of the sampling disk on the sensor —\nthe wider the aperture, the shallower the depth of field.\n\n- f/1.4  wide open — strong bokeh, very shallow focus\n- f/2.8\n- f/5.6\n- f/8    stopped down — sharp throughout, minimal bokeh\n- f/16",
			.completion_detail = "Aperture of the camera",
			.allow_multiple = false
		};

		v30.children["aperture_blades"] = TagSchema{
			.name = "aperture_blades",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::INT,
			.range = std::make_pair(0, 64),
			.hover_info = "Number of diaphragm blades",
			.completion_detail = "Number of diaphragm blades",
			.allow_multiple = false
		};

		v30.children["aperture_rotation"] = TagSchema{
			.name = "aperture_rotation",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.range = std::make_pair(0, 360),
			.hover_info = "Rotation of the bokeh polygon (degrees)",
			.completion_detail = "Rotation of the bokeh polygon",
			.allow_multiple = false
		};

		v30.children["shutter_speed"] = TagSchema{
			.name = "shutter_speed",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.hover_info = "Change the shutter speed (used for motion blur in animations)",
			.completion_detail = "Shutter speed",
			.allow_multiple = false
		};

		root.children["cameras"].children["camera"].variants.push_back(std::move(v30));
	}

	{
		ConditionalVariant v33;
		v33.discriminator_attr = "projection";
		v33.discriminator_value = "orthographic";
		v33.hover_info = "An orthographic camera and its properties.";
		v33.allow_text = false;

		v33.children["ortho_scale"] = TagSchema{
			.name = "ortho_scale",
			.required = false,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.hover_info = "Width of the visible zone (in world units)",
			.completion_detail = "Width of the visible zone",
			.allow_multiple = false
		};

		root.children["cameras"].children["camera"].variants.push_back(std::move(v33));
	}

	{
		ConditionalVariant v34;
		v34.discriminator_attr = "projection";
		v34.discriminator_value = "fisheye";
		v34.hover_info = "A fisheye camera and its properties.";
		v34.allow_text = false;

		v34.children["fisheye_fov"] = TagSchema{
			.name = "fisheye_fov",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.range = std::make_pair(1, 360),
			.hover_info = "FOV of the fisheye camera",
			.completion_detail = "Fisheye FOV",
			.allow_multiple = false
		};

		v34.children["fisheye_mapping"] = TagSchema{
			.name = "fisheye_mapping",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::ENUM,
			.enum_values = {"equidistant", "equisolid", "orthographic", "stereographic"},
			.hover_info = "Type of the fisheye mapping",
			.completion_detail = "Type of the fisheye mapping",
			.allow_multiple = false
		};

		root.children["cameras"].children["camera"].variants.push_back(std::move(v34));
	}

	{
		ConditionalVariant v35;
		v35.discriminator_attr = "projection";
		v35.discriminator_value = "panoramic";
		v35.hover_info = "A panoramic camera and its properties.";
		v35.allow_text = false;

		v35.children["panoramic_type"] = TagSchema{
			.name = "panoramic_type",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::ENUM,
			.enum_values = {"equirectangular", "mercator"},
			.hover_info = "Type of the panoramic projection",
			.completion_detail = "Type of the projection",
			.allow_multiple = false
		};

		root.children["cameras"].children["camera"].variants.push_back(std::move(v35));
	}

	root.children["render"] = TagSchema{
		.name = "render",
		.required = false,
		.allow_text = false,
		.hover_info = "Informations for the render",
		.completion_detail = "Render information",
		.allow_multiple = false
	};

	root.children["render"].children["width"] = TagSchema{
		.name = "width",
		.required = true,
		.allow_text = true,
		.text_type = ValueType::INT,
		.range = std::make_pair(0, 65535),
		.hover_info = "Width of the render",
		.completion_detail = "Render width",
		.allow_multiple = false
	};

	root.children["render"].children["height"] = TagSchema{
		.name = "height",
		.required = true,
		.allow_text = true,
		.text_type = ValueType::INT,
		.range = std::make_pair(0, 65535),
		.hover_info = "Height of the render",
		.completion_detail = "Render height",
		.allow_multiple = false
	};

	root.children["render"].children["camera"] = TagSchema{
		.name = "camera",
		.required = true,
		.allow_text = false,
		.hover_info = "Choosen camera for the render",
		.completion_detail = "Render camera",
		.allow_multiple = false
	};

	root.children["render"].children["camera"].attributes["ref"] = AttributeSchema{
		.name = "ref",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "ID of the camera used for the render",
		.completion_detail = "Render camera reference"
	};

	root.children["render"].children["samples"] = TagSchema{
		.name = "samples",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::INT,
		.range = std::make_pair(0, 2147483647),
		.hover_info = "Amount of samples for the render",
		.completion_detail = "Amount of samples for the render",
		.allow_multiple = false
	};

	root.children["render"].children["max_bounces"] = TagSchema{
		.name = "max_bounces",
		.required = true,
		.allow_text = true,
		.text_type = ValueType::INT,
		.range = std::make_pair(0, 10000),
		.hover_info = "Max amount of ray bounces",
		.completion_detail = "Max amount of ray bounces",
		.allow_multiple = false
	};

	root.children["render"].children["output"] = TagSchema{
		.name = "output",
		.required = false,
		.allow_text = true,
		.hover_info = "Render output file",
		.completion_detail = "Render output file",
		.allow_multiple = false
	};

	root.children["textures"] = TagSchema{
		.name = "textures",
		.required = false,
		.allow_text = false,
		.hover_info = "List of the availables textures",
		.completion_detail = "List of the availables textures",
		.allow_multiple = false
	};

	root.children["textures"].children["texture"] = TagSchema{
		.name = "texture",
		.required = false,
		.allow_text = false,
		.hover_info = "Texture properties",
		.completion_detail = "Texture properties",
		.allow_multiple = true
	};

	root.children["textures"].children["texture"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = false,
		.type = ValueType::ENUM,
		.default_value = "filepath",
		.enum_values = {"filepath", "checker_local", "checker_global"},
		.hover_info = "Type of the texture",
		.completion_detail = "Texture type"
	};

	root.children["textures"].children["texture"].attributes["name"] = AttributeSchema{
		.name = "name",
		.required = true,
		.type = ValueType::STRING,
		.hover_info = "Unique identifier for the texture. Used as a reference for materials using it",
		.completion_detail = "Texture unique identifier"
	};

	root.children["textures"].children["texture"].attributes["label"] = AttributeSchema{
		.name = "label",
		.required = false,
		.type = ValueType::STRING,
		.hover_info = "Display name of the texture",
		.completion_detail = "Display name"
	};

	{
		ConditionalVariant v36;
		v36.discriminator_attr = "type";
		v36.discriminator_value = "filepath";
		v36.hover_info = "Texture from file";
		v36.allow_text = false;

		v36.children["path"] = TagSchema{
			.name = "path",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::FILEPATH,
			.hover_info = "Path of the texture",
			.completion_detail = "Path of the texture",
			.allow_multiple = false
		};

		root.children["textures"].children["texture"].variants.push_back(std::move(v36));
	}

	{
		ConditionalVariant v37;
		v37.discriminator_attr = "type";
		v37.discriminator_value = "checker_global";
		v37.hover_info = "Global checker texture";
		v37.allow_text = false;

		v37.children["even"] = TagSchema{
			.name = "even",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::COLOR,
			.hover_info = "Even color of the checker",
			.completion_detail = "Even color of the checker",
			.allow_multiple = false
		};

		v37.children["odd"] = TagSchema{
			.name = "odd",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::COLOR,
			.hover_info = "Odd color of the checker",
			.completion_detail = "Odd color of the checker",
			.allow_multiple = false
		};

		v37.children["scale"] = TagSchema{
			.name = "scale",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.hover_info = "Scale of the checker",
			.completion_detail = "Scale of the checker",
			.allow_multiple = false
		};

		root.children["textures"].children["texture"].variants.push_back(std::move(v37));
	}

	{
		ConditionalVariant v38;
		v38.discriminator_attr = "type";
		v38.discriminator_value = "checker_local";
		v38.hover_info = "Local checker texture";
		v38.allow_text = false;

		v38.children["even"] = TagSchema{
			.name = "even",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::COLOR,
			.hover_info = "Even color of the checker",
			.completion_detail = "Even color of the checker",
			.allow_multiple = false
		};

		v38.children["odd"] = TagSchema{
			.name = "odd",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::COLOR,
			.hover_info = "Odd color of the checker",
			.completion_detail = "Odd color of the checker",
			.allow_multiple = false
		};

		v38.children["scale"] = TagSchema{
			.name = "scale",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::FLOAT,
			.hover_info = "Scale of the checker",
			.completion_detail = "Scale of the checker",
			.allow_multiple = false
		};

		root.children["textures"].children["texture"].variants.push_back(std::move(v38));
	}

	root.children["lights"] = TagSchema{
		.name = "lights",
		.required = false,
		.allow_text = false,
		.hover_info = "Container for all analytical lights in the scene.\nEmissive surfaces are defined at the material level and are not listed here.",
		.completion_detail = "Lights",
		.allow_multiple = false
	};

	root.children["lights"].children["light"] = TagSchema{
		.name = "light",
		.required = false,
		.allow_text = false,
		.hover_info = "Defines an analytical light source. The expected children depend on the type attribute.",
		.completion_detail = "Scene light",
		.allow_multiple = true
	};

	root.children["lights"].children["light"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = true,
		.type = ValueType::ENUM,
		.enum_values = {"point", "directional"},
		.hover_info = "Type of the light",
		.completion_detail = "Type of the light"
	};

	root.children["lights"].children["light"].children["color"] = TagSchema{
		.name = "color",
		.required = true,
		.allow_text = true,
		.text_type = ValueType::COLOR,
		.hover_info = "Spectral color of the emitted light.",
		.completion_detail = "Light color",
		.allow_multiple = false
	};

	root.children["lights"].children["light"].children["intensity"] = TagSchema{
		.name = "intensity",
		.required = false,
		.allow_text = true,
		.text_type = ValueType::FLOAT,
		.hover_info = "Power of the light source.\n\nFor point lights, intensity falls off with the inverse square of the distance.\nFor directional lights, intensity is uniform regardless of distance.\n\nMultiplied by color to give the final emitted radiance.",
		.completion_detail = "Light intensity",
		.allow_multiple = false
	};

	{
		ConditionalVariant v39;
		v39.discriminator_attr = "type";
		v39.discriminator_value = "point";
		v39.hover_info = "Omnidirectional light emitting equally in all directions from a single point.\nIntensity falls off with the square of the distance (inverse square law).";
		v39.allow_text = false;

		v39.children["position"] = TagSchema{
			.name = "position",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::VEC3,
			.hover_info = "World-space position of the light source.\n",
			.completion_detail = "Light position",
			.allow_multiple = false
		};

		root.children["lights"].children["light"].variants.push_back(std::move(v39));
	}

	{
		ConditionalVariant v40;
		v40.discriminator_attr = "type";
		v40.discriminator_value = "directional";
		v40.hover_info = "Infinitely distant light source emitting parallel rays in a uniform direction.\nModels large, far away sources such as the sun or moon.\nHas no position — only direction and intensity matter.";
		v40.allow_text = false;

		v40.children["direction"] = TagSchema{
			.name = "direction",
			.required = true,
			.allow_text = true,
			.text_type = ValueType::VEC3,
			.range = std::make_pair(-1, 1),
			.hover_info = "World-space direction the light is emitting toward, as a normalized vec3.\nFor example (0 -1 0) points straight down.",
			.completion_detail = "Light direction",
			.allow_multiple = false
		};

		root.children["lights"].children["light"].variants.push_back(std::move(v40));
	}

	root.children["environment"] = TagSchema{
		.name = "environment",
		.required = false,
		.allow_text = false,
		.hover_info = "Defines the background and ambient lighting of the scene.\nContributes to global illumination — rays that escape all geometry sample this instead.",
		.completion_detail = "Scene environment",
		.allow_multiple = false
	};

	root.children["environment"].attributes["type"] = AttributeSchema{
		.name = "type",
		.required = true,
		.type = ValueType::ENUM,
		.enum_values = {"skybox", "color"},
		.hover_info = "Type of the used environment",
		.completion_detail = "environment type"
	};

	{
		ConditionalVariant v41;
		v41.discriminator_attr = "type";
		v41.discriminator_value = "skybox";
		v41.hover_info = "Spherical environment map used as the scene background and indirect light source.\nThe texture is projected onto an infinite sphere.\nExpects a high dynamic range image (.exr, .hdr) for physically accurate lighting.";
		v41.allow_text = true;
		v41.text_type = ValueType::FILEPATH;

		root.children["environment"].variants.push_back(std::move(v41));
	}

	{
		ConditionalVariant v42;
		v42.discriminator_attr = "type";
		v42.discriminator_value = "color";
		v42.hover_info = "Radiance of the background, in linear RGB.\nValues above 1.0 are valid and act as an emissive background contributing to global illumination.\n\n- 0 0 0      black background, no ambient contribution\n- 1 1 1      white, uniformly lit scene\n- 0.5 0.7 1  light blue sky approximation";
		v42.allow_text = true;
		v42.text_type = ValueType::VEC3;

		root.children["environment"].variants.push_back(std::move(v42));
	}

}

}
