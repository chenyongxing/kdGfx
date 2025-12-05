#include "Scene.h"
#include "Project.h"
#include "StagingBuffer.h"
#include "ImageProcessor.h"
#include "MipMapsGen.h"

namespace kdGfx
{
	Node::~Node()
	{
		for (auto child : children)
		{
			delete child;
		}
	}

	bool Scene::init(const std::shared_ptr<Device>& device)
	{
		_device = device;

		return true;
	}

	void Scene::destroy()
	{
		materialsBuffer.reset();
		lightsBuffer.reset();
		instancesBuffer.reset();

		for (auto node : nodes)
		{
			delete node;
		}
		nodes.clear();
		images.clear();
		materials.clear();
		meshes.clear();

		_device.reset();
	}

	void Scene::update(float deltaTime)
	{
		if (_assetsDirty)
		{
			_uploadMeshes();
			_uploadImages();
			_uploadMaterials();

			spdlog::info("scene meshes updated. size = {}", meshes.size());
			spdlog::info("scene images updated. size = {}", images.size());
			spdlog::info("scene materials updated. size = {}", materials.size());
			Project::singleton()->eventTower.dispatchEvent(EventAssetsUpload);
			_assetsDirty = false;
		}

		if (_dirty)
		{
			_updateCacheNodes();
			_uploadNodes();

			spdlog::info("scene nodes updated. instance size = {}", drawCommandCount);
			Project::singleton()->eventTower.dispatchEvent(EventNodesChanged);
			_dirty = false;
		}
	}

	void Scene::traverseNodes(std::function<void(Node* node)> processNode, Node* node)
	{
		std::function<void(Node*)> traverseNode = [&](Node* _node)
			{
				processNode(_node);

				for (Node* __node : _node->children)
				{
					traverseNode(__node);
				}
			};

		if (node != nullptr)
		{
			traverseNode(node);
		}
		else
		{
			for (auto& node : nodes)
			{
				traverseNode(node);
			}
		}
	}

	void Scene::findNode(std::function<bool(Node* node)> condition, Node* node)
	{
		std::function<void(Node*)> traverseNode = [&](Node* _node)
			{
				if (condition(_node)) return;

				for (Node* __node : _node->children)
				{
					traverseNode(__node);
				}
			};

		if (node != nullptr)
		{
			traverseNode(node);
		}
		else
		{
			for (auto& node : nodes)
			{
				traverseNode(node);
			}
		}
	}

	void Scene::deleteNode(Node* node)
	{
		if (node == nullptr)	return;
		std::string modelRoot = node->modelRoot;

		std::vector<Node*>* _nodes = nullptr;
		if (node->parent != nullptr)
			_nodes = &node->parent->children;
		else
			_nodes = &this->nodes;

		auto it = std::find(_nodes->begin(), _nodes->end(), node);
		if (it != _nodes->end())
		{
			_nodes->erase(it);
			delete node;
		}
		
		markDirty();

		// 检查引用资源是否需要释放
		if (!modelRoot.empty())
		{
			bool haveRef = false;
			auto condition = [&haveRef, modelRoot](Node* node)
				{
					if (node->modelRoot == modelRoot)
					{
						haveRef = true;
						return true;
					}
					return false;
				};
			findNode(condition);
			if (!haveRef)
			{
				for (auto it = images.begin(); it != images.end();)
				{
					if ((*it)->assetFile == modelRoot)
						it = images.erase(it);
					else
						++it;
				}
				for (auto it = materials.begin(); it != materials.end();)
				{
					if ((*it)->assetFile == modelRoot)
						it = materials.erase(it);
					else
						++it;
				}
				for (auto it = meshes.begin(); it != meshes.end();)
				{
					if ((*it)->assetFile == modelRoot)
						it = meshes.erase(it);
					else
						++it;
				}
				markAssetsDirty();
			}
		}
	}

	void Scene::childNode(Node* parent, Node* node)
	{
		if (parent == nullptr || node == nullptr)	return;

		// B父亲移除B
		std::vector<Node*>* _nodes = nullptr;
		if (node->parent != nullptr)
			_nodes = &node->parent->children;
		else
			_nodes = &this->nodes;
		auto it = std::find(_nodes->begin(), _nodes->end(), node);
		if (it != _nodes->end())
		{
			_nodes->erase(it);
		}

		// B正成为A的孩子
		parent->children.emplace_back(node);
		node->parent = parent;

		markDirty();
	}

	void Scene::exchangeNode(Node* nodeA, Node* nodeB)
	{
		if (nodeA == nullptr || nodeB == nullptr)	return;

		// 同级交换
		if (nodeA->parent == nodeB->parent)
		{
			std::vector<Node*>* _nodes = nullptr;
			if (nodeA->parent != nullptr)
				_nodes = &nodeA->parent->children;
			else
				_nodes = &this->nodes;

			auto itA = std::find(_nodes->begin(), _nodes->end(), nodeA);
			auto itB = std::find(_nodes->begin(), _nodes->end(), nodeB);
			std::swap(*itA, *itB);
		}
		else // 不同级时候B成为A同级
		{
			std::vector<Node*>* _nodes = nullptr;
			// B父亲移除B
			if (nodeB->parent != nullptr)
				_nodes = &nodeB->parent->children;
			else
				_nodes = &this->nodes;
			auto it = std::find(_nodes->begin(), _nodes->end(), nodeB);
			if (it != _nodes->end())
			{
				_nodes->erase(it);
			}

			// A父亲添加B
			if (nodeA->parent != nullptr)
				_nodes = &nodeA->parent->children;
			else
				_nodes = &this->nodes;
			_nodes->emplace_back(nodeB);
			nodeB->parent = nodeA->parent;
		}

		markDirty();
	}

	void Scene::copyNode(Node* node, Node* parent)
	{
		if (node == nullptr)	return;

		std::function<Node* (Node*)> traverseNode = [&](Node* _node)
			{
				Node* copyed = _copyNodeInternal(_node);
				for (Node* __node : _node->children)
				{
					Node* copyedChild = traverseNode(__node);
					copyedChild->parent = copyed;
					copyed->children.emplace_back(copyedChild);
				}
				return copyed;
			};
		Node* copyed = traverseNode(node);

		if (parent == nullptr)
		{
			if (node->parent)
				childNode(node->parent, copyed);
			else
				nodes.emplace_back(copyed);
		}
		else
		{
			// 需要移除最上层节点，直接挂载到parent
			for (Node* child : copyed->children)
			{
				child->parent = parent;
				parent->children.emplace_back(child);
			}
			parent->modelRoot = copyed->modelRoot;
			copyed->children.clear();
			delete copyed;
		}

		markDirty();
	}

	void Scene::markMaterialChanged(Material* material)
	{
		for (uint32_t i = 0; i < materials.size(); i++)
		{
			if (material == materials[i].get())
			{
				MaterialGPU materialGpu;
				materialGpu.fromMaterial(material);
				MaterialGPU* ptr = (MaterialGPU*)materialsBuffer->map();
				memcpy(ptr + i, &materialGpu, sizeof(MaterialGPU));
				return;
			}
		}
	}

	void Scene::markLightChanged(Light* light)
	{
		LightGPU lightGPU;
		lightGPU.fromLight(light);
		LightGPU* ptr = (LightGPU*)lightsBuffer->map();
		memcpy(ptr + light->index, &lightGPU, sizeof(LightGPU));
	}

	void Scene::markNodeTransformed(Node* node)
	{
		if (node == nullptr)	return;

		auto processNode = [this](Node* node)
			{
				MeshInstance* meshInstance = dynamic_cast<MeshInstance*>(node);
				if (meshInstance)
				{
					_transformedMeshInstances.emplace(meshInstance);
				}

				Light* light = dynamic_cast<Light*>(node);
				if (light)
				{
					LightGPU* ptr = (LightGPU*)lightsBuffer->map();
					ptr += light->index;
					ptr->transform = node->getLocalToWorldTransform();
				}
			};
		traverseNodes(processNode, node);
	}

	void Scene::markNodeSelected(Node* node, bool _selected)
	{
		if (node == nullptr)	return;

		auto processNode = [this, _selected](Node* node)
			{
				MeshInstance* meshInstance = dynamic_cast<MeshInstance*>(node);
				if (meshInstance)
				{
					uint32_t intSelected = _selected ? 1 : 0;
					SubMeshInstanceGPU* ptr = (SubMeshInstanceGPU*)instancesBuffer->map();
					ptr += _meshInstanceIndicesMap[meshInstance];
					for (SubMesh& subMesh : meshInstance->mesh->subMeshes)
					{
						uint8_t* ptr2 = (uint8_t*)(ptr);
						memcpy(ptr2 + offsetof(SubMeshInstanceGPU, selected), &intSelected, sizeof(uint32_t));
						ptr++;
					}
				}
			};
		traverseNodes(processNode, node);
	}

	void Scene::_updateAssetsIndex()
	{
		for (uint32_t i = 0; i < images.size(); i++)	images[i]->index = i;
		for (uint32_t i = 0; i < materials.size(); i++)	materials[i]->index = i;

		uint32_t subMeshCount = 0;
		for (auto& mesh : meshes)
			for (auto& subMesh : mesh->subMeshes)
				subMesh.index = subMeshCount++;
	}

	void Scene::_uploadMeshes()
	{
		std::vector<SubMesh::Vertex> mergedVertices;
		std::vector<uint32_t> mergedIndices;
		for (auto& mesh : meshes)
		{
			for (auto& subMesh : mesh->subMeshes)
			{
				size_t vertexOffset = mergedVertices.size();
				size_t indexOffset = mergedIndices.size();
				mergedVertices.insert(mergedVertices.end(), subMesh.vertices.begin(), subMesh.vertices.end());

				for (uint32_t index : subMesh.indices)
				{
					mergedIndices.push_back(index + vertexOffset);
				}

				_subMeshIndexOffsetsMap[subMesh.index] = indexOffset;
			}

			mesh->dirty = false;
		}

		BufferDesc verticesBufferDesc;
		verticesBufferDesc.size = sizeof(SubMesh::Vertex) * mergedVertices.size();
		verticesBufferDesc.usage = BufferUsage::Vertex | BufferUsage::Storage | BufferUsage::CopyDst;
		verticesBufferDesc.name = "Vertices";
		verticesBuffer = _device->createBuffer(verticesBufferDesc);
		StagingBuffer::getUploadGlobal().uploadBuffer(verticesBuffer, mergedVertices.data(), verticesBufferDesc.size);

		BufferDesc indicesBufferDesc;
		indicesBufferDesc.size = sizeof(uint32_t) * mergedIndices.size();
		indicesBufferDesc.usage = BufferUsage::Index | BufferUsage::Storage | BufferUsage::CopyDst;
		indicesBufferDesc.name = "Indices";
		indicesBuffer = _device->createBuffer(indicesBufferDesc);
		StagingBuffer::getUploadGlobal().uploadBuffer(indicesBuffer, mergedIndices.data(), indicesBufferDesc.size);
	}

	void Scene::_uploadImages()
	{
		auto startTime = std::chrono::steady_clock::now();
		for (auto& image : images)
		{
			if (!image->dirty)	continue;
			TextureDesc desc;
			desc.usage = TextureUsage::CopyDst | TextureUsage::Sampled | TextureUsage::Storage;
			desc.name = image->name;
			desc.width = image->width;
			desc.height = image->height;
			desc.format = image->format;
			uint32_t fitMipLevel = (uint32_t)floorf(log2f(std::max(image->width, image->height))) + 1;
			desc.mipLevels = image->genMipmap ? fitMipLevel : 1;
			image->texture = _device->createTexture(desc);
			image->textureView = image->texture->createView({ .levelCount = desc.mipLevels });
			StagingBuffer::getUploadGlobal().uploadTexture(image->texture, image->data.data(), image->data.size());
			if (image->isSrgb)	ImageProcessor::singleton().process({ .gamma = 2.2f }, image->texture, 0);
			if (image->genMipmap) MipMapsGen::singleton().generate(image->texture);
			image->data.clear();
			image->dirty = false;
		}
		auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<float> timeDura = endTime - startTime;
		spdlog::info("scene upload images cost {}s", timeDura.count());
	}

	void Scene::_uploadMaterials()
	{
		materialsBuffer.reset();

		if (materials.empty()) return;

		std::vector<MaterialGPU> materialGPUs(materials.size());
		for (size_t i = 0; i < materialGPUs.size(); i++)
		{
			Material* material = materials[i].get();
			materialGPUs[i].fromMaterial(material);
		}

		BufferDesc desc;
		desc.size = sizeof(MaterialGPU) * materialGPUs.size();
		desc.stride = sizeof(MaterialGPU);
		desc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
		desc.name = "Materials";
		materialsBuffer = _device->createBuffer(desc);
		StagingBuffer::getUploadGlobal().uploadBuffer(materialsBuffer, materialGPUs.data(), materialsBuffer->getSize());

		for (auto& material : materials)	material->dirty = false;
	}

	void Scene::_uploadNodes()
	{
		std::vector<SubMeshInstanceGPU> instances;
		std::vector<LightGPU> lights;
		std::vector<DrawIndexedIndirectCommand> drawCommands;
		auto processNode = [&](Node* node)
			{
				MeshInstance* meshInstance = dynamic_cast<MeshInstance*>(node);
				if (meshInstance)
				{
					auto& subMeshes = meshInstance->mesh->subMeshes;
					for (uint32_t i = 0; i < subMeshes.size(); i++)
					{
						const auto& subMesh = subMeshes[i];
						SubMeshInstanceGPU instance;
						instance.transform = meshInstance->getWorldTransform();
						instance.subMeshIndex = subMesh.index;
						instance.materialIndex = meshInstance->materials[i]->index;
						instances.emplace_back(instance);

						drawCommands.push_back
						({
							.indexCount = (uint32_t)subMesh.indices.size(),
							.instanceCount = 1,
							.firstIndex = _subMeshIndexOffsetsMap.at(subMesh.index),
							.vertexOffset = 0,
							.firstInstance = (uint32_t)instances.size() - 1u // SV_StartInstanceLocation=instanceIndex
						});
					}
				}

				Light* light = dynamic_cast<Light*>(node);
				if (light)
				{
					LightGPU lightGPU;
					lightGPU.fromLight(light);
					lights.emplace_back(lightGPU);
				}
			};
		traverseNodes(processNode);

		// lights防止空Buffer，最后一位填充
		lights.emplace_back(LightGPU{});

		instancesBuffer.reset();
		if (instances.size() > 0)
		{
			BufferDesc desc;
			desc.size = sizeof(SubMeshInstanceGPU) * instances.size();
			desc.stride = sizeof(SubMeshInstanceGPU);
			desc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
			desc.name = "Scene-Instances";
			instancesBuffer = _device->createBuffer(desc);
			StagingBuffer::getUploadGlobal().uploadBuffer(instancesBuffer, instances.data(), instancesBuffer->getSize());
		}

		lightsBuffer.reset();
		if (lights.size() > 0)
		{
			BufferDesc desc;
			desc.size = sizeof(LightGPU) * lights.size();
			desc.stride = sizeof(LightGPU);
			desc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
			desc.name = "Scene-Lights";
			lightsBuffer = _device->createBuffer(desc);
			StagingBuffer::getUploadGlobal().uploadBuffer(lightsBuffer, lights.data(), lightsBuffer->getSize());
		}

		drawCommandsBuffer.reset();
		if (drawCommands.size() > 0)
		{
			BufferDesc desc;
			desc.size = sizeof(DrawIndexedIndirectCommand) * drawCommands.size();
			desc.stride = sizeof(DrawIndexedIndirectCommand);
			desc.usage = BufferUsage::Storage | BufferUsage::Indirect | BufferUsage::CopyDst;
			desc.name = "Scene-DrawCommands";
			drawCommandsBuffer = _device->createBuffer(desc);
			StagingBuffer::getUploadGlobal().uploadBuffer(drawCommandsBuffer, drawCommands.data(), drawCommandsBuffer->getSize());
		}
		drawCommandCount = drawCommands.size();
	}

	void Scene::_updateCacheNodes()
	{
		_meshInstanceIndicesMap.clear();
		cameras.clear();

		std::unordered_map<uint32_t, MeshInstance*> instanceIdMeshInstanceMap;
		_subMeshesCount = 0;
		size_t lightCount = 0;
		auto processNode = [this, &instanceIdMeshInstanceMap, &lightCount](Node* node)
			{
				MeshInstance* meshInstance = dynamic_cast<MeshInstance*>(node);
				if (meshInstance)
				{
					_meshInstanceIndicesMap[meshInstance] = _subMeshesCount;
					for (SubMesh& subMesh : meshInstance->mesh->subMeshes)
					{
						instanceIdMeshInstanceMap[_subMeshesCount] = meshInstance;
						_subMeshesCount++;
					}
				}

				Camera* camera = dynamic_cast<Camera*>(node);
				if (camera)
				{
					cameras.emplace_back(camera);
				}

				Light* light = dynamic_cast<Light*>(node);
				if (light)
				{
					light->index = lightCount++;
				}
			};
		traverseNodes(processNode);

		_instanceIdModelMap.clear();
		for (auto pair : instanceIdMeshInstanceMap)
		{
			uint32_t instanceId = pair.first;
			MeshInstance* meshInstance = pair.second;
			// 向上查找第一个modelRoot
			Node* parent = meshInstance->parent;
			while (parent != nullptr && parent->modelRoot.empty())
			{
				parent = parent->parent;
			}
			_instanceIdModelMap[instanceId] = parent;
		}
	}

	Node* Scene::_copyNodeInternal(Node* node)
	{
		if (!node) return nullptr;

		Node* copyed = nullptr;

		MeshInstance* meshInstance = dynamic_cast<MeshInstance*>(node);
		Camera* camera = dynamic_cast<Camera*>(node);
		Light* light = dynamic_cast<Light*>(node);
		if (meshInstance)
		{
			MeshInstance* copyedMeshInstance = new MeshInstance();
			copyedMeshInstance->mesh = meshInstance->mesh;
			copyedMeshInstance->materials = meshInstance->materials;
			copyed = copyedMeshInstance;
		}
		else if (camera)
		{
			Camera* copyedCamera = new Camera();
			copyedCamera->type = camera->type;
			copyedCamera->fovY = camera->fovY;
			copyedCamera->aspect = camera->aspect;
			copyedCamera->nearZ = camera->nearZ;
			copyedCamera->farZ = camera->farZ;
			copyedCamera->focalDistance = camera->focalDistance;
			copyedCamera->aperture = camera->aperture;
			copyed = copyedCamera;
		}
		else if (light)
		{
			DirectionalLight* directionalLight = dynamic_cast<DirectionalLight*>(light);
			if (directionalLight)
			{
				DirectionalLight* copyedDirectionalLight = new DirectionalLight();
				copyedDirectionalLight->color = directionalLight->color;
				copyedDirectionalLight->angularDiameter = directionalLight->angularDiameter;
			}
		}
		else
		{
			copyed = new Node();
		}

		copyed->modelRoot = node->modelRoot;
		copyed->name = node->name;
		copyed->translation = node->translation;
		copyed->scale = node->scale;
		copyed->rotation = node->rotation;
		return copyed;
	}
}
