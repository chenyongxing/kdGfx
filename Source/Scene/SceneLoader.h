#pragma once

#include "Scene.h"
#include <tiny_gltf.h>

namespace kdGfx
{
	class SceneLoader final
	{
	public:
		bool load();
		void save();
		bool addModel(const std::string& relativePath, Node* root);
		bool setHDRI(const std::string& relativePath);

	private:
		void _setNodeProperty(Node* node, const tinygltf::Node& gltfNode);

		void _loadImages(const tinygltf::Model& model, std::vector<std::shared_ptr<Image>>& images);

		void _loadMaterials(const tinygltf::Model& model, 
			std::vector<std::shared_ptr<Material>>& materials, 
			std::vector<std::shared_ptr<Image>>& images);

		void _loadMeshes(const tinygltf::Model& model, std::vector<std::shared_ptr<Mesh>>& meshes);

		void _loadNodes(const tinygltf::Model& model, Node* root,
			std::vector<std::shared_ptr<Image>>& images,
			std::vector<std::shared_ptr<Material>>& materials,
			std::vector<std::shared_ptr<Mesh>>& meshes);
	};
}
