#include "SceneLoader.h"
#include "Project.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace fs = std::filesystem;

namespace kdGfx
{
	bool SceneLoader::load()
	{
		std::string path(Project::singleton()->getRootPath());
		path.append("/project.scene");

		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string error;
		std::string warn;
		bool result = loader.LoadASCIIFromFile(&model, &error, &warn, path);
		if (!result)
		{
			spdlog::warn("failed to load project.scene");
			return false;
		}

		if (model.scenes.empty())	return false;

		// 全局环境参数
		auto extIt = model.extensions.find("KD_env_params");
		auto& params = extIt->second;
		if (extIt != model.extensions.end() && params.IsObject())
		{
			if (params.Has("hdri_uri"))
			{
				std::string hdriUri = extIt->second.Get("hdri_uri").Get<std::string>();
				setHDRI(hdriUri);
			}
			if (params.Has("intensity"))
			{
			}
		}

		const tinygltf::Scene& gltfScene = model.scenes[0];
		for (size_t i = 0; i < gltfScene.nodes.size(); i++)
		{
			const tinygltf::Node& gltfNode = model.nodes[i];

			Node* node = nullptr;
			if (gltfNode.light >= 0)
			{
				const tinygltf::Light& gltfLight = model.lights[gltfNode.light];
				if (gltfLight.type == "directional")
				{
					auto light = new DirectionalLight();
					light->color = { (float)gltfLight.color[0], (float)gltfLight.color[1], (float)gltfLight.color[2] };
					node = light;
				}
			}
			else if (gltfNode.camera >= 0)
			{
				const tinygltf::Camera& gltfCamera = model.cameras[gltfNode.camera];
				if (gltfCamera.type == "perspective")
				{
					auto camera = new Camera();
					camera->type = Camera::Type::Perspective;
					camera->fovY = (float)glm::degrees(gltfCamera.perspective.yfov);
					camera->aspect = (float)gltfCamera.perspective.aspectRatio;
					camera->nearZ = (float)gltfCamera.perspective.znear;
					camera->farZ = (float)gltfCamera.perspective.zfar;
					node = camera;
				}
			}
			else
			{
				// 扩展模型节点
				node = new Node();
				for (auto extPair : gltfNode.extensions)
				{
					if (extPair.first == "KD_node_gltf")
					{
						if (extPair.second.Has("uri"))
						{
							auto uri = extPair.second.Get("uri").Get<std::string>();
							addModel(uri, node);
						}
					}
				}
			}
			
			if (node) 
			{
				_setNodeProperty(node, gltfNode);
				Project::singleton()->getScene()->nodes.emplace_back(node);
			}
		}

		Project::singleton()->getScene()->markAssetsDirty();
		Project::singleton()->getScene()->markDirty();
		return true;
	}

	void SceneLoader::save()
	{
		tinygltf::TinyGLTF saver;
		tinygltf::Model gltfModel;
		gltfModel.scenes.resize(1);
		tinygltf::Scene& scene = gltfModel.scenes[0];

		std::function<int(Node*, tinygltf::Node*)> traverseNode = [&](Node* node, tinygltf::Node* gltfParent)
			{
				tinygltf::Node gltfNode;
				gltfNode.name = node->name;
				gltfNode.translation.resize(3);
				gltfNode.translation[0] = node->translation.x;
				gltfNode.translation[1] = node->translation.y;
				gltfNode.translation[2] = node->translation.z;
				gltfNode.scale.resize(3);
				gltfNode.scale[0] = node->scale.x;
				gltfNode.scale[1] = node->scale.y;
				gltfNode.scale[2] = node->scale.z;
				gltfModel.nodes.emplace_back(gltfNode);
				int gltfNodeIndex = gltfModel.nodes.size() - 1;

				if (gltfParent)	gltfParent->children.emplace_back(gltfNodeIndex);

				if (node->modelRoot.empty())
				{
					for (Node* _node : node->children)
					{
						traverseNode(_node, &gltfModel.nodes.back());
					}
				}
				else
				{
					std::map<std::string, tinygltf::Value> uri;
					uri["uri"] = tinygltf::Value(node->modelRoot);

					auto& extensions = gltfModel.nodes.back().extensions;
					extensions["KD_node_gltf"] = tinygltf::Value(uri);
				}

				return gltfNodeIndex;
			};
		if (Project::singleton()->getScene())
		{
			for (auto& node : Project::singleton()->getScene()->nodes)
			{
				int gltfNodeIndex = traverseNode(node, nullptr);
				scene.nodes.emplace_back(gltfNodeIndex);
			}
		}
		
		std::string path(Project::singleton()->getRootPath());
		path.append("/project.scene");
		saver.WriteGltfSceneToFile(&gltfModel, path, false, false, true, false);
	}

	bool SceneLoader::addModel(const std::string& relativePath, Node* root)
	{
		root->modelRoot.clear();
		auto scene = Project::singleton()->getScene();
		// 已经有一份了。复制实例化
		Node* modelRoot = nullptr;
		auto condition = [&modelRoot, relativePath](Node* node)
			{
				if (node->modelRoot == relativePath)
				{
					modelRoot = node;
					return true;
				}
				return false;
			};
		scene->findNode(condition);
		if (modelRoot)
		{
			scene->copyNode(modelRoot, root);
			return true;
		}

		std::string path(Project::singleton()->getRootPath());
		path.append("/");
		path.append(relativePath);

		tinygltf::TinyGLTF loader;
		tinygltf::Model gltfModel;
		std::string error;
		std::string warn;
		bool result = false;
		if (fs::path(path).extension().string() == ".glb")
			result = loader.LoadBinaryFromFile(&gltfModel, &error, &warn, path);
		else
			result = loader.LoadASCIIFromFile(&gltfModel, &error, &warn, path);
		if (!result)
		{
			spdlog::warn("failed to load model: {}", path);
			return false;
		}

		if (gltfModel.scenes.empty())	return false;

		// 暂未实现当前场景按需加载资源。当前为加载全部资源
		std::vector<std::shared_ptr<Image>> images;
		_loadImages(gltfModel, images);
		for (auto image : images)	image->assetFile = relativePath;

		std::vector<std::shared_ptr<Material>> materials;
		_loadMaterials(gltfModel, materials, images);
		for (auto material : materials)	material->assetFile = relativePath;

		std::vector<std::shared_ptr<Mesh>> meshes;
		_loadMeshes(gltfModel, meshes);
		for (auto mesh : meshes)	mesh->assetFile = relativePath;

		_loadNodes(gltfModel, root, images, materials, meshes);
		root->modelRoot = relativePath;

		// 资源附加到场景
		scene->images.insert(scene->images.end(), images.begin(), images.end());
		scene->materials.insert(scene->materials.end(), materials.begin(), materials.end());
		scene->meshes.insert(scene->meshes.end(), meshes.begin(), meshes.end());
		scene->markAssetsDirty();
		scene->markDirty();
		return true;
	}

	bool SceneLoader::setHDRI(const std::string& relativePath)
	{
		Scene* scene = Project::singleton()->getScene();
		if (scene->HDRI && scene->HDRI->assetFile == relativePath)	return false;

		std::string path(Project::singleton()->getRootPath());
		path.append("/");
		path.append(relativePath);

		int width = 0;
		int height = 0;
		int component = 0;
		float* imageData = stbi_loadf(path.c_str(), &width, &height, &component, STBI_rgb_alpha);
		if (imageData == nullptr)	return false;

		auto image = std::make_shared<Image>();
		image->dirty = true;
		image->assetFile = relativePath;
		image->name = fs::path(path).stem().string();
		image->format = Format::RGBA32Sfloat;
		image->width = (uint32_t)width;
		image->height = (uint32_t)height;
		image->genMipmap = false;
		size_t byteSize = width * height * sizeof(float) * 4;
		image->data.resize(byteSize);
		memcpy(image->data.data(), imageData, byteSize);
		STBI_FREE(imageData);
		
		scene->images.erase(std::remove_if(scene->images.begin(), scene->images.end(),
			[scene](std::shared_ptr<Image> image)
			{
				return scene->HDRI == image.get();
			}), scene->images.end());
		scene->images.push_back(image);
		scene->markAssetsDirty();
		scene->HDRI = image.get();
		return true;
	}

	void SceneLoader::_setNodeProperty(Node* node, const tinygltf::Node& gltfNode)
	{
		if (!gltfNode.name.empty())	node->name = gltfNode.name;

		if (gltfNode.scale.size() == 3)
		{
			glm::vec3 scale = { (float)gltfNode.scale[0], (float)gltfNode.scale[1], (float)gltfNode.scale[2] };
			node->scale = scale;
		}
		if (gltfNode.rotation.size() == 4)
		{
			glm::quat quatRoation;
			quatRoation.x = (float)gltfNode.rotation[0];
			quatRoation.y = (float)gltfNode.rotation[1];
			quatRoation.z = (float)gltfNode.rotation[2];
			quatRoation.w = (float)gltfNode.rotation[3];
			node->rotation = quatRoation;
		}
		if (gltfNode.translation.size() == 3)
		{
			glm::vec3 translation = { (float)gltfNode.translation[0], (float)gltfNode.translation[1], (float)gltfNode.translation[2] };
			node->translation = translation;
		}
	}

	void SceneLoader::_loadImages(const tinygltf::Model& model, std::vector<std::shared_ptr<Image>>& images)
	{
		std::unordered_set<int> baseColorTextures;
		for (auto material : model.materials)
		{
			if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
			{
				baseColorTextures.emplace(material.pbrMetallicRoughness.baseColorTexture.index);
			}
		}

		images.reserve(model.textures.size());
		for (size_t i = 0; i < model.textures.size(); i++)
		{
			const tinygltf::Texture& gltfTexture = model.textures[i];
			auto image = std::make_shared<Image>();
			image->name = gltfTexture.name;
			if (gltfTexture.source >= 0)
			{
				const tinygltf::Image& gltfImage = model.images[gltfTexture.source];
				if (image->name.empty()) image->name = gltfImage.name;
				if (image->name.empty()) image->name = gltfImage.uri;
				if (gltfImage.image.size() > 0)
				{
					if (gltfImage.bits == 8 && gltfImage.component == 4)
					{
						image->format = Format::RGBA8Unorm;
						image->width = (uint32_t)gltfImage.width;
						image->height = (uint32_t)gltfImage.height;
						image->data.resize(gltfImage.image.size());
						std::copy(gltfImage.image.begin(), gltfImage.image.end(), image->data.begin());
						// 如果当作baseColor需要设置成sRGB
						if (baseColorTextures.count(i) > 0)	image->isSrgb = true;
					}
					else if (gltfImage.bits == 16 && gltfImage.component == 4)
					{
						image->format = Format::RGBA16Unorm;
						image->width = (uint32_t)gltfImage.width;
						image->height = (uint32_t)gltfImage.height;
						image->data.resize(gltfImage.image.size());
						std::copy(gltfImage.image.begin(), gltfImage.image.end(), image->data.begin());
					}
					else if (gltfImage.bits == 32 && gltfImage.component == 4)
					{
						image->format = Format::RGBA32Sfloat;
						image->width = (uint32_t)gltfImage.width;
						image->height = (uint32_t)gltfImage.height;
						image->data.resize(gltfImage.image.size());
						std::copy(gltfImage.image.begin(), gltfImage.image.end(), image->data.begin());
					}
					else
					{
						spdlog::error("[gltfLoader] image format not support. {}", image->name);
					}
				}
			}
			// 错误贴图
			if (image->data.size() == 0)
			{
				image->width = 32;
				image->height = 32;
				size_t byteSize = image->width * image->height * 4;
				image->data.resize(byteSize);
				memset(image->data.data(), 255, byteSize);
			}
			images.push_back(image);
		}
	}

	void SceneLoader::_loadMaterials(const tinygltf::Model& model,
		std::vector<std::shared_ptr<Material>>& materials,
		std::vector<std::shared_ptr<Image>>& images)
	{
		auto getImage = [images](int i) -> Image*
			{
				if (i >= 0 && i < images.size())
				{
					return images[i].get();
				}
				return nullptr;
			};

		materials.reserve(model.materials.size());
		for (size_t i = 0; i < model.materials.size(); i++)
		{
			const tinygltf::Material& gltfMaterial = model.materials[i];
			const tinygltf::PbrMetallicRoughness& pbr = gltfMaterial.pbrMetallicRoughness;

			auto material = std::make_shared<Material>();
			material->name = gltfMaterial.name;

			material->emissive = { (float)gltfMaterial.emissiveFactor[0],
				(float)gltfMaterial.emissiveFactor[1], (float)gltfMaterial.emissiveFactor[2] };
			material->emissiveMap = getImage(gltfMaterial.emissiveTexture.index);
			material->baseColor = { (float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1],
				(float)pbr.baseColorFactor[2], (float)pbr.baseColorFactor[3] };
			if (gltfMaterial.alphaMode == "BLEND") material->alphaMode = Material::AlphaMode::Blend;
			else if (gltfMaterial.alphaMode == "MASK") material->alphaMode = Material::AlphaMode::Mask;
			material->alphaCutoff = (float)gltfMaterial.alphaCutoff;
			material->roughness = (float)pbr.roughnessFactor;
			material->metallic = (float)pbr.metallicFactor;
			material->baseColorMap = getImage(pbr.baseColorTexture.index);
			material->occlusionRoughnessMetallicMap = getImage(pbr.metallicRoughnessTexture.index);
			material->normalMap = getImage(gltfMaterial.normalTexture.index);
			for (auto extPair : gltfMaterial.extensions)
			{
				if (extPair.first == "KHR_materials_ior")
				{
					if (extPair.second.Has("ior"))
					{
						float ior = (float)extPair.second.Get("ior").GetNumberAsDouble();
						material->ior = std::max(0.01f, ior);
					}
				}
				else if (extPair.first == "KHR_materials_transmission")
				{
					if (extPair.second.Has("transmissionFactor"))
					{
						float transmissionFactor = (float)extPair.second.Get("transmissionFactor").GetNumberAsDouble();
						material->transmission = std::clamp(transmissionFactor, 0.0f, 1.0f);
					}
				}
				else if (extPair.first == "KHR_materials_volume")
				{
					if (extPair.second.Has("attenuationColor"))
					{
						auto attenuationColor = extPair.second.Get("attenuationColor");
						material->attenuationColor =
						{
							(float)attenuationColor.Get(0).GetNumberAsDouble(),
							(float)attenuationColor.Get(1).GetNumberAsDouble(),
							(float)attenuationColor.Get(2).GetNumberAsDouble()
						};
					}
					if (extPair.second.Has("attenuationDistance"))
					{
						float attenuationDistance = (float)extPair.second.Get("attenuationDistance").GetNumberAsDouble();
						material->attenuationDistance = std::max(0.01f, attenuationDistance);
					}
				}
				else if (extPair.first == "KHR_materials_emissive_strength")
				{
					if (extPair.second.Has("emissiveStrength"))
					{
						float emissiveStrength = (float)extPair.second.Get("emissiveStrength").GetNumberAsDouble();
						material->emissiveStrength = std::max(0.0f, emissiveStrength);
					}
				}
			}
			materials.emplace_back(material);
		}
	}

	void SceneLoader::_loadMeshes(const tinygltf::Model& model, std::vector<std::shared_ptr<Mesh>>& meshes)
	{
		meshes.reserve(model.meshes.size());
		for (size_t i = 0; i < model.meshes.size(); i++)
		{
			const tinygltf::Mesh& gltfMesh = model.meshes[i];
			if (gltfMesh.primitives.size() == 0) continue;

			auto mesh = std::make_shared<Mesh>();
			mesh->name = gltfMesh.name;
			mesh->subMeshes.resize(gltfMesh.primitives.size());

			for (size_t j = 0; j < gltfMesh.primitives.size(); j++)
			{
				const tinygltf::Primitive& glTFPrimitive = gltfMesh.primitives[j];
				SubMesh& subMesh = mesh->subMeshes[j];

				//vertices
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
					const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
					int stride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

					subMesh.vertices.resize(accessor.count);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
					{
						glm::vec3 position;
						for (size_t index = 0; index < accessor.count; index++)
						{
							const uint8_t* ptr = dataPtr + (index * stride);
							memcpy(&position, ptr, stride);
							subMesh.vertices[index].position = position;
						}
					}
					else
					{
						spdlog::error("[gltfLoader] POSITION format not support");
						return;
					}
				}
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
					const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
					int stride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
					{
						glm::vec3 normal;
						for (size_t index = 0; index < accessor.count; index++)
						{
							const uint8_t* ptr = dataPtr + (index * stride);
							memcpy(&normal, ptr, stride);
							subMesh.vertices[index].normal = normal;
						}
					}
					else
					{
						spdlog::error("[gltfLoader] NORMAL format not support");
						return;
					}
				}
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
					const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
					int stride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2)
					{
						glm::vec2 texCoord;
						for (size_t index = 0; index < accessor.count; index++)
						{
							const uint8_t* ptr = dataPtr + (index * stride);
							memcpy(&texCoord, ptr, stride);
							subMesh.vertices[index].texCoord = texCoord;
						}
					}
					else
					{
						spdlog::error("[gltfLoader] TEXCOORD_0 format not support");
						return;
					}
				}

				//indices
				{
					tinygltf::Accessor accessor = model.accessors[glTFPrimitive.indices];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
					const uint8_t* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

					subMesh.indices.resize(accessor.count);
					int stride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && accessor.type == TINYGLTF_TYPE_SCALAR)
					{
						std::vector<uint8_t> byte;
						byte.resize(accessor.count);
						memcpy(byte.data(), dataPtr, (accessor.count * stride));
						for (size_t i = 0; i < accessor.count; i++)
						{
							subMesh.indices[i] = byte[i];
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT && accessor.type == TINYGLTF_TYPE_SCALAR)
					{
						std::vector<uint16_t> half;
						half.resize(accessor.count);
						memcpy(half.data(), dataPtr, (accessor.count * stride));
						for (size_t i = 0; i < accessor.count; i++)
						{
							subMesh.indices[i] = half[i];
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT && accessor.type == TINYGLTF_TYPE_SCALAR)
					{
						memcpy(subMesh.indices.data(), dataPtr, (accessor.count * stride));
					}
					else
					{
						spdlog::error("[gltfLoader] indices format not support");
						return;
					}
				}
			}

			meshes.emplace_back(mesh);
		}
	}

	static void ParseNodes(const tinygltf::Model& model, std::vector<Node*>& nodes,
		std::vector<std::shared_ptr<Image>>& images,
		std::vector<std::shared_ptr<Material>>& materials,
		std::vector<std::shared_ptr<Mesh>>& meshes)
	{
		std::vector<Camera*> cameras;
		cameras.reserve(model.cameras.size());
		for (size_t i = 0; i < model.cameras.size(); i++)
		{
			const tinygltf::Camera& gltfCamera = model.cameras[i];
			auto camera = new Camera();
			if (gltfCamera.type == "perspective")
			{
				camera->type = Camera::Type::Perspective;
				camera->fovY = (float)glm::degrees(gltfCamera.perspective.yfov);
				camera->aspect = (float)gltfCamera.perspective.aspectRatio;
				camera->nearZ = (float)gltfCamera.perspective.znear;
				camera->farZ = (float)gltfCamera.perspective.zfar;
				cameras.emplace_back(camera);
			}
		}

		std::vector<Light*> lights;
		lights.reserve(model.lights.size());
		for (size_t i = 0; i < model.lights.size(); i++)
		{
			const tinygltf::Light& gltfLight = model.lights[i];
			if (gltfLight.type == "directional")
			{
				auto light = new DirectionalLight();
				light->color = { (float)gltfLight.color[0], (float)gltfLight.color[1], (float)gltfLight.color[2] };
				lights.emplace_back(light);
			}
		}

		nodes.reserve(model.nodes.size());
		for (const tinygltf::Node& gltfNode : model.nodes)
		{
			Node* node = nullptr;
			if (gltfNode.mesh >= 0)
			{
				node = new MeshInstance();
			}
			else if (gltfNode.light >= 0)
			{
				node = lights[gltfNode.light];
			}
			else if (gltfNode.camera >= 0)
			{
				node = cameras[gltfNode.camera];
			}
			else
			{
				node = new Node();
			}

			if (!gltfNode.name.empty())	node->name = gltfNode.name;

			if (gltfNode.matrix.size() == 16)
			{
				glm::mat4 transform =
				{
					gltfNode.matrix[0], gltfNode.matrix[1], gltfNode.matrix[2], gltfNode.matrix[3],
					gltfNode.matrix[4], gltfNode.matrix[5], gltfNode.matrix[6], gltfNode.matrix[7],
					gltfNode.matrix[8], gltfNode.matrix[9], gltfNode.matrix[10], gltfNode.matrix[11],
					gltfNode.matrix[12], gltfNode.matrix[13], gltfNode.matrix[14], gltfNode.matrix[15]
				};
				node->setLocalTransform(transform);
			}
			else
			{
				if (gltfNode.scale.size() == 3)
				{
					glm::vec3 scale = { (float)gltfNode.scale[0], (float)gltfNode.scale[1], (float)gltfNode.scale[2] };
					node->scale = scale;
				}
				if (gltfNode.rotation.size() == 4)
				{
					glm::quat quatRoation;
					quatRoation.x = (float)gltfNode.rotation[0];
					quatRoation.y = (float)gltfNode.rotation[1];
					quatRoation.z = (float)gltfNode.rotation[2];
					quatRoation.w = (float)gltfNode.rotation[3];
					node->rotation = quatRoation;
				}
				if (gltfNode.translation.size() == 3)
				{
					glm::vec3 translation = { (float)gltfNode.translation[0], (float)gltfNode.translation[1], (float)gltfNode.translation[2] };
					node->translation = translation;
				}
			}

			if (gltfNode.mesh >= 0 && gltfNode.mesh < meshes.size())
			{
				auto meshInstance = dynamic_cast<MeshInstance*>(node);
				meshInstance->mesh = meshes[gltfNode.mesh].get();

				const tinygltf::Mesh& gltfMesh = model.meshes[gltfNode.mesh];
				meshInstance->materials.reserve(gltfMesh.primitives.size());
				for (size_t i = 0; i < gltfMesh.primitives.size(); i++)
				{
					tinygltf::Primitive glTFPrimitive = gltfMesh.primitives[i];
					meshInstance->materials.emplace_back(materials[glTFPrimitive.material].get());
				}
			}

			nodes.emplace_back(std::move(node));
		}

		for (size_t i = 0; i < model.nodes.size(); i++)
		{
			const tinygltf::Node& gltfNode = model.nodes[i];
			Node* node = nodes[i];
			//关联node父子关系
			for (size_t j = 0; j < gltfNode.children.size(); j++)
			{
				Node* child = nodes[gltfNode.children[j]];
				child->parent = node;
				node->children.push_back(child);
			}
		}
	}

	void SceneLoader::_loadNodes(const tinygltf::Model& model, Node* root,
		std::vector<std::shared_ptr<Image>>& images,
		std::vector<std::shared_ptr<Material>>& materials,
		std::vector<std::shared_ptr<Mesh>>& meshes)
	{
		std::vector<Node*> nodes;
		ParseNodes(model, nodes, images, materials, meshes);

		for (auto nodeIndex : model.scenes[0].nodes)
		{
			Node* child = nodes[nodeIndex];
			child->parent = root;
			root->children.emplace_back(child);
		}

		// TODO: 删除没有被场景引用的节点
	}
}
