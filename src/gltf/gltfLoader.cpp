#include "gltfLoader.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace {

	struct DataDeleter {
		void operator()(cgltf_data* data) const { cgltf_free(data); }
	};

	// glTF stores matrices column by column; mat4 is m[row][col].
	mat4 toMatrix(const cgltf_float* values) {
		mat4 m(0.0f);
		for (int col = 0; col < 4; ++col)
			for (int row = 0; row < 4; ++row)
				m.m[row][col] = values[col * 4 + row];
		return m;
	}

	quat rotationOf(const mat4& m) {
		const float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];
		quat q;
		if (trace > 0.0f) {
			const float s = std::sqrt(trace + 1.0f) * 2.0f;
			q = quat(0.25f * s, (m.m[2][1] - m.m[1][2]) / s, (m.m[0][2] - m.m[2][0]) / s, (m.m[1][0] - m.m[0][1]) / s);
		} else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
			const float s = std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f;
			q = quat((m.m[2][1] - m.m[1][2]) / s, 0.25f * s, (m.m[0][1] + m.m[1][0]) / s, (m.m[0][2] + m.m[2][0]) / s);
		} else if (m.m[1][1] > m.m[2][2]) {
			const float s = std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f;
			q = quat((m.m[0][2] - m.m[2][0]) / s, (m.m[0][1] + m.m[1][0]) / s, 0.25f * s, (m.m[1][2] + m.m[2][1]) / s);
		} else {
			const float s = std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f;
			q = quat((m.m[1][0] - m.m[0][1]) / s, (m.m[0][2] + m.m[2][0]) / s, (m.m[1][2] + m.m[2][1]) / s, 0.25f * s);
		}
		return q.normalized();
	}

	void readNode(const cgltf_node& source, ModelNode& node) {
		if (!source.has_matrix) {
			if (source.has_translation)
				node.translation = {source.translation[0], source.translation[1], source.translation[2]};
			if (source.has_rotation)
				node.rotation = quat(source.rotation[3], source.rotation[0], source.rotation[1], source.rotation[2]);
			if (source.has_scale)
				node.scale = {source.scale[0], source.scale[1], source.scale[2]};
			return;
		}

		mat4 m = toMatrix(source.matrix);
		node.translation = {m.m[0][3], m.m[1][3], m.m[2][3]};
		float scale[3];
		for (int col = 0; col < 3; ++col) {
			scale[col] = std::sqrt(m.m[0][col] * m.m[0][col] + m.m[1][col] * m.m[1][col] + m.m[2][col] * m.m[2][col]);
			for (int row = 0; row < 3 && scale[col] > 0.0f; ++row)
				m.m[row][col] /= scale[col];
		}
		node.scale	  = {scale[0], scale[1], scale[2]};
		node.rotation = rotationOf(m);
	}

	const cgltf_accessor* findAttribute(const cgltf_primitive& primitive, cgltf_attribute_type type) {
		for (cgltf_size i = 0; i < primitive.attributes_count; ++i)
			if (primitive.attributes[i].type == type && primitive.attributes[i].index == 0)
				return primitive.attributes[i].data;
		return nullptr;
	}

	// Weights as bytes adding up to exactly 255, the rounding error going to the heaviest bone.
	void packWeights(const float weights[4], VertexSkin& skin) {
		const float total = weights[0] + weights[1] + weights[2] + weights[3];
		if (total <= 0.0f) {
			skin.weights[0] = 255;
			return;
		}

		int sum		 = 0;
		int heaviest = 0;
		for (int i = 0; i < 4; ++i) {
			skin.weights[i] = static_cast<uint8_t>(std::clamp(weights[i] / total, 0.0f, 1.0f) * 255.0f + 0.5f);
			sum += skin.weights[i];
			if (skin.weights[i] > skin.weights[heaviest])
				heaviest = i;
		}
		skin.weights[heaviest] = static_cast<uint8_t>(skin.weights[heaviest] + 255 - sum);
	}

	// Smooth normals for a mesh that comes without any.
	void computeNormals(ModelMesh& mesh) {
		for (Vertex& vertex : mesh.vertices)
			vertex.normal = vec3(0.0f);
		for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
			Vertex& a = mesh.vertices[mesh.indices[i]];
			Vertex& b = mesh.vertices[mesh.indices[i + 1]];
			Vertex& c = mesh.vertices[mesh.indices[i + 2]];
			const vec3 normal = vec3::cross(b.pos - a.pos, c.pos - a.pos);
			a.normal = a.normal + normal;
			b.normal = b.normal + normal;
			c.normal = c.normal + normal;
		}
		for (Vertex& vertex : mesh.vertices)
			vertex.normal = vertex.normal.length() > 0.0f ? vec3::normalize(vertex.normal) : vec3(0.0f, 1.0f, 0.0f);
	}

	std::string nameOr(const char* name, const char* fallback, size_t index) {
		return name && *name ? std::string(name) : fallback + std::to_string(index);
	}

	// Reads the clips of a file; `nodeOf` gives the model node a glTF node drives, or -1.
	template <typename NodeLookup>
	void readClips(const cgltf_data* raw, const NodeLookup& nodeOf, std::vector<AnimationClip>& clips) {
		for (cgltf_size a = 0; a < raw->animations_count; ++a) {
			const cgltf_animation& source = raw->animations[a];
			AnimationClip		   clip;
			clip.name = nameOr(source.name, "clip_", a);

			for (cgltf_size c = 0; c < source.channels_count; ++c) {
				const cgltf_animation_channel& from = source.channels[c];
				if (!from.target_node || !from.sampler || !from.sampler->input || !from.sampler->output)
					continue;

				AnimationChannel channel;
				channel.node = nodeOf(from.target_node);
				if (channel.node < 0)
					continue;
				if (from.target_path == cgltf_animation_path_type_translation)
					channel.path = AnimationChannel::Path::Translation;
				else if (from.target_path == cgltf_animation_path_type_rotation)
					channel.path = AnimationChannel::Path::Rotation;
				else if (from.target_path == cgltf_animation_path_type_scale)
					channel.path = AnimationChannel::Path::Scale;
				else
					continue;

				// A cubic spline stores (in tangent, value, out tangent) for each key: only the
				// values are kept, and played as a linear curve.
				const bool		 cubic = from.sampler->interpolation == cgltf_interpolation_type_cubic_spline;
				const cgltf_size keys  = from.sampler->input->count;
				const cgltf_size width = channel.path == AnimationChannel::Path::Rotation ? 4 : 3;
				if (from.sampler->output->count < keys * (cubic ? 3 : 1))
					continue;

				channel.step = from.sampler->interpolation == cgltf_interpolation_type_step;
				channel.times.resize(keys);
				channel.values.resize(keys);
				for (cgltf_size k = 0; k < keys; ++k) {
					cgltf_float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
					cgltf_accessor_read_float(from.sampler->input, k, &channel.times[k], 1);
					cgltf_accessor_read_float(from.sampler->output, cubic ? k * 3 + 1 : k, values, width);
					channel.values[k] = vec4(values[0], values[1], values[2], values[3]);
				}
				if (keys > 0)
					clip.duration = std::max(clip.duration, channel.times.back());
				clip.channels.push_back(std::move(channel));
			}
			if (!clip.channels.empty())
				clips.push_back(std::move(clip));
		}
	}

	cgltf_data* parse(const std::string& path, std::string& error) {
		cgltf_options options{};
		cgltf_data*	  raw = nullptr;
		if (cgltf_parse_file(&options, path.c_str(), &raw) != cgltf_result_success) {
			error = "cannot read '" + path + "' as glTF";
			return nullptr;
		}
		if (cgltf_load_buffers(&options, raw, path.c_str()) != cgltf_result_success || cgltf_validate(raw) != cgltf_result_success) {
			cgltf_free(raw);
			error = "'" + path + "' is not a valid glTF file";
			return nullptr;
		}
		return raw;
	}

}

bool sceneIO::loadGltf(const std::string& path, Model& model, std::string& error) {
	cgltf_data* raw = parse(path, error);
	if (!raw)
		return false;
	std::unique_ptr<cgltf_data, DataDeleter> data(raw);

	model	   = Model{};
	model.name = std::filesystem::path(path).stem().string();

	// --- Nodes, parents first.
	std::unordered_map<const cgltf_node*, int> nodeIndex;
	const auto addNode = [&](const auto& self, const cgltf_node* source, int parent) -> void {
		const int index	   = static_cast<int>(model.nodes.size());
		nodeIndex[source]  = index;
		ModelNode node;
		node.name	= nameOr(source->name, "node_", static_cast<size_t>(index));
		node.parent = parent;
		readNode(*source, node);
		model.nodes.push_back(std::move(node));
		for (cgltf_size i = 0; i < source->children_count; ++i)
			self(self, source->children[i], index);
	};
	for (cgltf_size i = 0; i < raw->nodes_count; ++i)
		if (!raw->nodes[i].parent)
			addNode(addNode, &raw->nodes[i], -1);

	// --- Bones: the joints of every skin, then one per node carrying a rigid mesh.
	std::unordered_map<const cgltf_skin*, size_t> skinBase;
	for (cgltf_size i = 0; i < raw->skins_count; ++i) {
		const cgltf_skin& skin = raw->skins[i];
		skinBase[&skin]		   = model.bones.size();
		for (cgltf_size joint = 0; joint < skin.joints_count; ++joint) {
			ModelBone bone;
			bone.node = nodeIndex[skin.joints[joint]];
			cgltf_float values[16];
			if (skin.inverse_bind_matrices && cgltf_accessor_read_float(skin.inverse_bind_matrices, joint, values, 16))
				bone.inverseBind = toMatrix(values);
			model.bones.push_back(bone);
		}
	}
	std::unordered_map<int, size_t> rigidBone;
	const auto boneOfNode = [&](int node) {
		auto it = rigidBone.find(node);
		if (it != rigidBone.end())
			return it->second;
		model.bones.push_back(ModelBone{node, mat4::identity()});
		return rigidBone[node] = model.bones.size() - 1;
	};

	// --- Materials: base color only, as a texture or a flat color. Images used for anything
	// else (normal maps, roughness) are left out: they would be loaded for nothing.
	std::vector<bool> baseColor(raw->images_count, false);
	for (cgltf_size i = 0; i < raw->materials_count; ++i) {
		const cgltf_texture* texture = raw->materials[i].has_pbr_metallic_roughness ? raw->materials[i].pbr_metallic_roughness.base_color_texture.texture : nullptr;
		if (texture && texture->image)
			baseColor[static_cast<size_t>(texture->image - raw->images)] = true;
	}
	for (cgltf_size i = 0; i < raw->images_count; ++i) {
		const cgltf_image& image = raw->images[i];
		ModelTexture	   texture;
		texture.name = "image_" + std::to_string(i);
		if (!baseColor[i]) {
			model.textures.push_back(std::move(texture)); // kept empty so that indices still match
			continue;
		}
		if (image.buffer_view) {
			const uint8_t* bytes = cgltf_buffer_view_data(image.buffer_view);
			if (bytes)
				texture.encoded.assign(bytes, bytes + image.buffer_view->size);
		} else if (image.uri && std::string(image.uri).rfind("data:", 0) != 0) {
			std::string uri = image.uri;
			uri.resize(cgltf_decode_uri(uri.data()));
			texture.path = (std::filesystem::path(path).parent_path() / uri).string();
		}
		model.textures.push_back(std::move(texture));
	}

	const auto usable = [&](const cgltf_texture* texture) {
		if (!texture || !texture->image)
			return false;
		const ModelTexture& image = model.textures[static_cast<size_t>(texture->image - raw->images)];
		return !image.encoded.empty() || !image.path.empty();
	};
	for (cgltf_size i = 0; i < raw->materials_count; ++i) {
		const cgltf_material& source = raw->materials[i];
		Material			  material;
		material.name = "material_" + std::to_string(i);
		material.label = source.name ? source.name : "";

		const cgltf_float* color = source.pbr_metallic_roughness.base_color_factor;
		if (source.has_pbr_metallic_roughness && usable(source.pbr_metallic_roughness.base_color_texture.texture)) {
			const cgltf_image* image = source.pbr_metallic_roughness.base_color_texture.texture->image;
			material.albedo = MaterialParam<vec3>{model.textures[static_cast<size_t>(image - raw->images)].name};
		} else if (source.has_pbr_metallic_roughness) {
			material.albedo = MaterialParam<vec3>{vec3(color[0], color[1], color[2])};
		} else {
			material.albedo = MaterialParam<vec3>{vec3(0.8f)};
		}

		const float emission = std::max({source.emissive_factor[0], source.emissive_factor[1], source.emissive_factor[2]});
		if (emission > 0.0f)
			material.emission_strength = MaterialParam<float>{std::min(emission, 1.0f)};
		model.materials.push_back(std::move(material));
	}
	bool needsDefault = false;

	// --- Meshes: one per primitive, in model space or bound to the bones.
	for (cgltf_size n = 0; n < raw->nodes_count; ++n) {
		const cgltf_node& node = raw->nodes[n];
		if (!node.mesh)
			continue;

		for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
			const cgltf_primitive& primitive = node.mesh->primitives[p];
			const cgltf_accessor*  positions = findAttribute(primitive, cgltf_attribute_type_position);
			if (primitive.type != cgltf_primitive_type_triangles || !positions)
				continue;

			const cgltf_accessor* normals = findAttribute(primitive, cgltf_attribute_type_normal);
			const cgltf_accessor* uvs	  = findAttribute(primitive, cgltf_attribute_type_texcoord);
			const cgltf_accessor* joints  = node.skin ? findAttribute(primitive, cgltf_attribute_type_joints) : nullptr;
			const cgltf_accessor* weights = node.skin ? findAttribute(primitive, cgltf_attribute_type_weights) : nullptr;
			const bool			  skinned = joints && weights;

			ModelMesh mesh;
			mesh.name = nameOr(node.mesh->name, "mesh_", static_cast<size_t>(n)) + "_" + std::to_string(p);
			if (primitive.material) {
				mesh.material = "material_" + std::to_string(primitive.material - raw->materials);
			} else {
				mesh.material = "default";
				needsDefault  = true;
			}

			const size_t rigid = skinned ? 0 : boneOfNode(nodeIndex[&node]);
			const size_t base  = skinned ? skinBase[node.skin] : 0;
			if (model.bones.size() > 256) {
				error = "'" + path + "' has more than 256 bones";
				return false;
			}

			mesh.vertices.resize(positions->count);
			mesh.skin.resize(positions->count);
			for (cgltf_size i = 0; i < positions->count; ++i) {
				Vertex&		vertex = mesh.vertices[i];
				cgltf_float values[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				cgltf_accessor_read_float(positions, i, values, 3);
				vertex.pos = {values[0], values[1], values[2]};
				if (normals && cgltf_accessor_read_float(normals, i, values, 3))
					vertex.normal = {values[0], values[1], values[2]};
				vertex.uv = {0.5f, 0.5f};
				if (uvs && cgltf_accessor_read_float(uvs, i, values, 2))
					vertex.uv = {values[0], values[1]};

				VertexSkin& skin = mesh.skin[i];
				if (!skinned) {
					skin.bones[0]	= static_cast<uint8_t>(rigid);
					skin.weights[0] = 255;
					continue;
				}
				cgltf_uint indices[4] = {0, 0, 0, 0};
				cgltf_accessor_read_uint(joints, i, indices, 4);
				cgltf_accessor_read_float(weights, i, values, 4);
				for (int k = 0; k < 4; ++k)
					skin.bones[k] = static_cast<uint8_t>(base + std::min<size_t>(indices[k], node.skin->joints_count - 1));
				packWeights(values, skin);
			}

			if (primitive.indices) {
				mesh.indices.resize(primitive.indices->count);
				for (cgltf_size i = 0; i < primitive.indices->count; ++i)
					mesh.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(primitive.indices, i));
			} else {
				mesh.indices.resize(positions->count);
				for (size_t i = 0; i < mesh.indices.size(); ++i)
					mesh.indices[i] = static_cast<uint32_t>(i);
			}
			mesh.indices.resize(mesh.indices.size() / 3 * 3);
			if (std::any_of(mesh.indices.begin(), mesh.indices.end(), [&](uint32_t index) { return index >= mesh.vertices.size(); }))
				continue;

			if (!normals)
				computeNormals(mesh);

			// Primitives of one mesh often share one large vertex array: keep what this one uses.
			std::vector<uint32_t> remap(mesh.vertices.size(), UINT32_MAX);
			std::vector<Vertex>		used;
			std::vector<VertexSkin> usedSkin;
			for (uint32_t& index : mesh.indices) {
				if (remap[index] == UINT32_MAX) {
					remap[index] = static_cast<uint32_t>(used.size());
					used.push_back(mesh.vertices[index]);
					usedSkin.push_back(mesh.skin[index]);
				}
				index = remap[index];
			}
			mesh.vertices = std::move(used);
			mesh.skin	  = std::move(usedSkin);
			model.meshes.push_back(std::move(mesh));
		}
	}
	if (needsDefault) {
		Material material;
		material.name	= "default";
		material.albedo = MaterialParam<vec3>{vec3(0.8f)};
		model.materials.push_back(std::move(material));
	}
	if (model.meshes.empty()) {
		error = "'" + path + "' has no triangle mesh";
		return false;
	}
	std::erase_if(model.textures, [](const ModelTexture& texture) { return texture.encoded.empty() && texture.path.empty(); });

	// --- Clips.
	readClips(raw, [&](const cgltf_node* node) { return nodeIndex[node]; }, model.clips);
	return true;
}

bool sceneIO::loadGltfAnimations(const std::string& path, Model& model, std::string& error, const std::string& clipName) {
	cgltf_data* raw = parse(path, error);
	if (!raw)
		return false;
	std::unique_ptr<cgltf_data, DataDeleter> data(raw);

	// The two files only share the names of their nodes.
	std::unordered_map<std::string, int> byName;
	for (size_t i = 0; i < model.nodes.size(); ++i)
		byName.emplace(model.nodes[i].name, static_cast<int>(i));

	std::vector<AnimationClip> clips;
	readClips(raw, [&](const cgltf_node* node) {
		const auto it = node->name ? byName.find(node->name) : byName.end();
		return it != byName.end() ? it->second : -1;
	}, clips);
	if (clips.empty()) {
		error = "'" + path + "' has no animation for this model";
		return false;
	}

	for (size_t i = 0; i < clips.size(); ++i) {
		if (!clipName.empty())
			clips[i].name = clips.size() == 1 ? clipName : clipName + " " + std::to_string(i);
		model.clips.push_back(std::move(clips[i]));
	}
	return true;
}
