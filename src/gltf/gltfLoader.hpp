#pragma once

#include "animation/Model.hpp"

#include <string>

namespace sceneIO {

	// Reads a glTF 2.0 file (.gltf or .glb): meshes, materials (base color only), skeleton and
	// animation clips. Meshes that are not skinned follow the node they hang from, so models
	// made of rigid parts animate too. Morph targets, cameras and lights are ignored.
	// Returns false and fills `error` when the file cannot be used.
	bool loadGltf(const std::string& path, Model& model, std::string& error);

	// Adds the clips of another glTF file to `model`: animations are often delivered one per
	// file, without the mesh. The nodes of the two files are matched by name; what the file
	// animates and the model does not have is dropped. A non-empty `clipName` renames the clip.
	bool loadGltfAnimations(const std::string& path, Model& model, std::string& error, const std::string& clipName = "");

}
