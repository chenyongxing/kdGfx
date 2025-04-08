#pragma once

#include "RHI/Device.h"

namespace kdGfx
{
	struct Image final
	{
		bool dirty = true;
		std::string assetFile;
		// Scene里面的索引
		uint32_t index = 0;
		std::string name;

		uint32_t width = 0;
		uint32_t height = 0;
		Format format = Format::Undefined;
		std::vector<uint8_t> data;
		bool isSrgb = false; // GPU直接使用SRGB格式不支持写入，需要预处理
		bool genMipmap = true;
		std::shared_ptr<Texture> texture;
		std::shared_ptr<TextureView> textureView;
	};

	struct Material final
	{
		enum struct AlphaMode
		{
			Opaque = 0,
			Blend = 1,
			Mask = 2
		};

		bool dirty = true;
		std::string assetFile;
		// Scene里面的索引
		uint32_t index = 0;
		std::string name;

		glm::vec4 baseColor{ 1.0f };
		Image* baseColorMap = nullptr;

		float metallic = 0.0f;
		float roughness = 0.5f;
		Image* occlusionRoughnessMetallicMap = nullptr;

		glm::vec3 sheenColor{ 1.0f };
		float sheenRoughness = 0.0f;
		float clearcoat = 0.0f;
		float clearcoatRoughness = 0.0f;

		AlphaMode alphaMode = AlphaMode::Opaque;
		float alphaCutoff = 0.5f;

		float ior = 1.5f;
		float transmission = 0.0f;
		glm::vec3 attenuationColor{ 1.0f };
		float attenuationDistance = 1.0f;

		glm::vec3 emissive{ 0.0f };
		Image* emissiveMap = nullptr;
		float emissiveStrength = 1.0f;

		Image* normalMap = nullptr;

		inline bool operator==(const Material& other) const
		{
			bool diff = 
				!EqualNearly(baseColor, other.baseColor) ||
				baseColorMap != other.baseColorMap ||
				occlusionRoughnessMetallicMap != other.occlusionRoughnessMetallicMap ||
				emissiveMap != other.emissiveMap ||
				normalMap != other.normalMap;
			return !diff;
		}
	};

	struct Mesh;
	struct SubMesh final
	{
		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 texCoord;
		};
		// Scene里面的索引
		size_t index = 0;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	struct Mesh final
	{
		bool dirty = true;
		std::string assetFile;
		std::string name;

		std::vector<SubMesh> subMeshes;
	};
}
