#include "RenderGraphRegistry.h"
#include "RenderGraph.h"

namespace kdGfx
{
	RenderGraphRegistry::RenderGraphRegistry(RenderGraph& graph) : _graph(graph)
	{
	}

	RenderGraphResource RenderGraphRegistry::createBuffer(const BufferDesc& desc)
	{
		RenderGraphResource index = ++_resourcesCount;
		_buffersMap[index] = std::make_tuple(desc, nullptr);
		_graph._createResourceEdge(desc.name, index);
		return index;
	}

	RenderGraphResource RenderGraphRegistry::createTexture(const TextureDesc& desc)
	{
		RenderGraphResource index = ++_resourcesCount;
		_texturesMap[index] = std::make_tuple(desc, 0, nullptr, nullptr);
		_graph._createResourceEdge(desc.name, index);
		return index;
	}

	RenderGraphResource RenderGraphRegistry::importBuffer(const std::shared_ptr<Buffer>& buffer, const std::string_view name)
	{
		RenderGraphResource index = ++_resourcesCount;
		_importBuffersMap[index] = buffer;
		_graph._createResourceEdge(name, index);
		return index;
	}

	RenderGraphResource RenderGraphRegistry::importTexture(const std::shared_ptr<Texture>& texture, const std::string_view name)
	{
		RenderGraphResource index = ++_resourcesCount;
		_importTexturesMap[index] = std::make_tuple(texture, texture->createView({}));
		_graph._createResourceEdge(name, index);
		return index;
	}

	RenderGraphResource RenderGraphRegistry::importTexture(const std::shared_ptr<Texture>& texture, const std::shared_ptr<TextureView>& textureView, const std::string_view name)
	{
		RenderGraphResource index = ++_resourcesCount;
		_importTexturesMap[index] = std::make_tuple(texture, textureView);
		_graph._createResourceEdge(name, index);
		return index;
	}

	void RenderGraphRegistry::setImportedTexture(RenderGraphResource handle, const std::shared_ptr<Texture>& texture, const std::shared_ptr<TextureView>& textureView)
	{
		if (auto it = _importTexturesMap.find(handle); it != _importTexturesMap.end())
		{
			auto& entry = it->second;
			std::get<0>(entry) = texture;
			std::get<1>(entry) = textureView;
		}
	}

	std::shared_ptr<Buffer> RenderGraphRegistry::getBuffer(RenderGraphResource handle)
	{
		if (_buffersMap.contains(handle))	return std::get<1>(_buffersMap[handle]);
		if (_importBuffersMap.contains(handle))	return _importBuffersMap[handle];
		return nullptr;
	}

	std::tuple<std::shared_ptr<Texture>, std::shared_ptr<TextureView>> RenderGraphRegistry::getTexture(RenderGraphResource handle)
	{
		if (_texturesMap.contains(handle))
		{
			return std::make_tuple(std::get<2>(_texturesMap[handle]), std::get<3>(_texturesMap[handle]));
		}
		if (_importTexturesMap.contains(handle))	return _importTexturesMap[handle];
		return std::make_tuple(nullptr, nullptr);
	}

	void RenderGraphRegistry::destroy()
	{
		_buffersMap.clear();
		_texturesMap.clear();

		_importBuffersMap.clear();
		_importTexturesMap.clear();
	}

	void RenderGraphRegistry::resize(uint32_t width, uint32_t height)
	{
		for (auto& texturePair : _texturesMap)
		{
			auto writer = _graph._getResourceEdge(texturePair.first)->producer;
			if (writer->type == RenderGraphPassType::FrameRaster || writer->type == RenderGraphPassType::FrameCompute)
			{
				auto& [textureDesc, size, texture, textureView] = texturePair.second;
				textureDesc.width = width;
				textureDesc.height = height;
				texture = _graph._device->createTexture(textureDesc);
				textureView = texture->createView({});
			}
		}
	}
}
