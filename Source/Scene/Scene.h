#pragma once

#include "Node.h"

namespace kdGfx
{
	class Scene
	{
	public:
		// assets
		std::vector<std::shared_ptr<Mesh>> meshes;
		std::vector<std::shared_ptr<Image>> images;
		std::vector<std::shared_ptr<Material>> materials;
		// node
		std::vector<Node*> nodes;
		std::vector<Camera*> cameras;
		// env light
		Image* HDRI = nullptr;

		std::shared_ptr<Buffer> verticesBuffer;
		std::shared_ptr<Buffer> indicesBuffer;
		std::shared_ptr<Buffer> materialsBuffer;
		std::shared_ptr<Buffer> instancesBuffer;
		std::shared_ptr<Buffer> lightsBuffer;
		std::shared_ptr<Buffer> drawCommandsBuffer;
		uint32_t drawCommandCount = 0;

	public:
		bool init(const std::shared_ptr<Device>& device);
		void destroy();
		void update(float deltaTime);

		void traverseNodes(std::function<void(Node* node)> processNode, Node* node = nullptr);
		void findNode(std::function<bool(Node* node)> condition, Node* node = nullptr);
		void deleteNode(Node* node);
		void childNode(Node* parent, Node* node);
		void exchangeNode(Node* nodeA, Node* nodeB);
		void copyNode(Node* node, Node* parent = nullptr);

		void markMaterialChanged(Material* material);
		void markLightChanged(Light* light);
		void markNodeTransformed(Node* node);
		void markNodeSelected(Node* node, bool selected = true);

		inline void markAssetsDirty() 
		{
			_updateAssetsIndex();
			_assetsDirty = true; 
		}
		inline void markDirty() { _dirty = true; }
		inline bool empty() { return instancesBuffer == nullptr; }
		inline Node* getModelRoot(uint32_t instanceId) 
		{
			if (_instanceIdModelMap.count(instanceId) > 0)
				return _instanceIdModelMap[instanceId];
			return nullptr;
		}
		
	private:
		std::shared_ptr<Device> _device;

		// 节点改变
		bool _dirty = false;
		// 资源改变
		bool _assetsDirty = false;
		
		uint32_t _subMeshesCount = 0;
		// submesh在总顶点索引里面的偏移
		std::unordered_map<uint32_t, uint32_t> _subMeshIndexOffsetsMap;
		// MeshInstance对应全部submesh的开始index
		std::unordered_map<MeshInstance*, uint32_t> _meshInstanceIndicesMap;
		// instanceId -> modelRoot
		std::unordered_map<uint32_t, Node*> _instanceIdModelMap;
		// 每帧更新变换矩阵的MeshInstance
		std::unordered_set<MeshInstance*> _transformedMeshInstances;

		void _updateAssetsIndex();
		void _uploadMeshes();
		void _uploadImages();
		void _uploadMaterials();
		
		// 更新MeshInstance和Light节点
		void _uploadNodes();
		void _updateCacheNodes();
		// 复制节点但是不包含子父
		Node* _copyNodeInternal(Node* node);
	};

#pragma pack(push, 16)
	struct SubMeshInstanceGPU
	{
		glm::mat4 transform{ 1.0f };
		uint32_t subMeshIndex = 0;
		uint32_t materialIndex = 0;
		uint32_t selected = 0;
		float _pad0 = 0;
	};
#pragma pack(pop)

#pragma pack(push, 16)
	struct MaterialGPU
	{
		glm::vec3 emissive;
		int emissiveMapIndex;
		glm::vec4 baseColor;
		int alphaMode;
		float alphaCutoff;
		uint32_t baseColorMapIndex = UINT32_MAX;
		float metallic;
		float roughness;
		uint32_t occlusionRoughnessMetallicMapIndex = UINT32_MAX;
		uint32_t normalMapIndex = UINT32_MAX;
		float ior;
		float sheenRoughness;
		float clearcoat;
		float clearcoatRoughness;
		float transmission;
		glm::vec3 attenuationColor;
		float attenuationDistance;
		glm::vec3 sheenColor;
		float _pad0;

		inline void fromMaterial(Material* material)
		{
			baseColor = material->baseColor;
			if (material->baseColorMap)
				baseColorMapIndex = material->baseColorMap->index;
			metallic = material->metallic;
			roughness = material->roughness;
			if (material->occlusionRoughnessMetallicMap)
				occlusionRoughnessMetallicMapIndex = material->occlusionRoughnessMetallicMap->index;
			if (material->normalMap)
				normalMapIndex = material->normalMap->index;
			transmission = material->transmission;
			ior = material->ior;
		}
	};
#pragma pack(pop)

#pragma pack(push, 16)
	struct LightGPU
	{
		int type = 0; //0=Directional, 1=Point, 2=Spot
		glm::mat4 transform{ 1.0f };
		glm::vec3 radiance;
		// dir
		float cosAngle = 1.0f; //cos(deg2rad(0.5f * angularDiameter))
		float invArea = 1.0f; //pdf=1/area
		// point
		float radius = 1.0f;
		//rect
		glm::vec3 corner;
		glm::vec3 u;
		glm::vec3 v;

		inline void fromLight(Light* light)
		{
			transform = light->getLocalToWorldTransform();
			radiance = light->color * light->intensity;

			DirectionalLight* directionalLight = dynamic_cast<DirectionalLight*>(light);
			if (directionalLight)
			{
				type = 0;
				float angularDiameter = glm::clamp(directionalLight->angularDiameter, 0.f, 180.f);
				cosAngle = cosf(glm::radians(0.5f * angularDiameter));
			}
		}
	};
#pragma pack(pop)

}
