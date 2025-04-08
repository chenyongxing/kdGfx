#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "../Misc.h"
#include <imnodes/imnodes_internal.h>
#include <set>

namespace kdGfx
{
	RenderGraph::RenderGraph(const std::shared_ptr<Device>& device) :
		_device(device),
		_registry(*this)
	{
	}

	RenderGraph::~RenderGraph()
	{
		for (auto node : _passes)
			delete node;
		_passes.clear();
		for (auto pair : _resourceEdgesMap)
			delete pair.second;
		_resourceEdgesMap.clear();
		_registry.destroy();

		_commandList.reset();
		_device.reset();
	}

	void RenderGraph::addPass(const std::string_view name, RenderGraphPassType type,
		RenderGraphPassNodeConstruction construct, bool isCullBase)
	{
		auto passNode = new RenderGraphPassNode();
		passNode->name = name;
		passNode->type = type;
		RenderGraphBuilder builder(*this, passNode);
		// 执行pass构造。生成DAG连接关系
		passNode->evaluation = construct(builder);
		passNode->isCullBase = isCullBase;
		_passes.emplace_back(passNode);
	}

	void RenderGraph::compile()
	{
		if (!_dirty) return;

		// 确定Pass之间的依赖
		for (auto& pass : _passes)
		{
			pass->dependencies.clear();
			for (auto& resEdge : pass->inputs)
			{
				if (resEdge->producer != nullptr)
				{
					pass->dependencies.emplace_back(resEdge->producer);
				}
			}
		}

		// 剔除掉对设定节点的无关联节点
		_cullPasses();

		// 生成并行pass执行列表
		_genParallelTasks();

		// 自动推导贴图Usage
		_autoSetTextureUsage();

		// 确定资源生命周期
		for (uint32_t i = 0; i < _passesTasks.size(); ++i)
		{
			for (auto pass : _passesTasks[i])
			{
				for (auto resEdge : pass->outputs)
				{
					resEdge->lifecycle.firstUseTask = i;
				}
				for (auto resEdge : pass->inputs)
				{
					resEdge->lifecycle.lastUseTask = i;
				}
			}
		}

		// 设置帧图大小
		for (auto& texturePair : _registry._texturesMap)
		{
			auto resEdge = _getResourceEdge(texturePair.first);
			auto& [textureDesc, size, texture, textureView] = texturePair.second;
			if (resEdge->producer->type == RenderGraphPassType::FrameRaster || 
				resEdge->producer->type == RenderGraphPassType::FrameCompute)
			{
				textureDesc.width = _width;
				textureDesc.height = _height;
			}
		}

		// buffer直接创建
		_allocResources();
		// texture支持内存别名
		_allocMemory();
		_createResources();
		
		_compiled = true;
		_dirty = false;
	}

	void RenderGraph::execute()
	{
		if (!_compiled || !_commandList) return;

		for (uint32_t i = 0; i < _passesTasks.size(); ++i)
		{
			for (auto& pass : _passesTasks[i])
			{
				_commandList->beginLabel(fmt::format("Pass {}", pass->name));
				_autoTransitionTextureState(pass, _commandList);
				pass->evaluation(_registry, *_commandList.get());
				_commandList->endLabel();
			}
		}
	}

	void RenderGraph::resize(uint32_t width, uint32_t height)
	{
		_width = width;
		_height = height;
		if (_compiled)
		{
			_registry.resize(width, height);
			spdlog::debug("render graph resize to {}x{}", _width, _height);
		}
	}

	void RenderGraph::_cullPasses()
	{
		// 找到需要保留的资源和Pass
		std::vector<RenderGraphPassNode*> cullBasePasses;
		for (auto pass : _passes)
		{
			if (pass->isCullBase)
			{
				cullBasePasses.push_back(pass);
			}
		}

		std::unordered_set<RenderGraphPassNode*> keepPasses;
		std::unordered_set<RenderGraphResourceEdge*> keepResources;
		std::queue<RenderGraphPassNode*> queue;

		for (auto pass : cullBasePasses)
		{
			keepPasses.insert(pass);
			for (auto output : pass->outputs)
			{
				keepResources.insert(output);
			}
			queue.push(pass);
		}

		// 逆向遍历依赖
		while (!queue.empty())
		{
			auto currentPass = queue.front();
			queue.pop();

			for (auto inputRes : currentPass->inputs)
			{
				keepResources.insert(inputRes);

				auto producer = inputRes->producer;
				if (producer && !keepPasses.count(producer))
				{
					keepPasses.insert(producer);
					queue.push(producer);
				}
			}
		}

		// 裁剪
		_passes.erase(std::remove_if(_passes.begin(), _passes.end(),
			[&](RenderGraphPassNode* pass)
			{
				return !keepPasses.count(pass);
			}),
			_passes.end());

		for (auto it = _resourceEdgesMap.begin(); it != _resourceEdgesMap.end();)
		{
			if (!keepResources.count(it->second))
			{
				delete it->second;
				it = _resourceEdgesMap.erase(it);
			}
			else
			{
				++it;
			}
		}

		// 清理被移除Pass的依赖关系
		for (auto pass : _passes)
		{
			auto& deps = pass->dependencies;
			deps.erase(std::remove_if(deps.begin(), deps.end(),
				[&](RenderGraphPassNode* dep)
				{
					return !keepPasses.count(dep);
				}),
			deps.end());
		}
	}

	void RenderGraph::_genParallelTasks()
	{
		std::unordered_map<RenderGraphPassNode*, size_t> inDegree;
		std::queue<RenderGraphPassNode*> queue;

		for (auto pass : _passes)
		{
			inDegree[pass] = pass->dependencies.size();
			if (inDegree[pass] == 0)
			{
				queue.push(pass);
			}
		}

		// 层级遍历
		while (!queue.empty())
		{
			size_t levelSize = queue.size();
			std::vector<RenderGraphPassNode*> currentLevel;

			// 处理当前层级所有节点
			for (size_t i = 0; i < levelSize; ++i)
			{
				auto currentPass = queue.front();
				queue.pop();
				currentLevel.emplace_back(currentPass);

				// 更新后续节点的入度
				for (auto outputRes : currentPass->outputs)
				{
					for (auto consumerPass : outputRes->consumers)
					{
						if (--inDegree[consumerPass] == 0)
						{
							queue.push(consumerPass);
						}
					}
				}
			}

			_passesTasks.emplace_back(std::move(currentLevel));
		}

		spdlog::info("render graph {} - pass tasks = [", name);
		for (uint32_t i = 0; i < _passesTasks.size(); ++i)
		{
			auto& passesTask = _passesTasks[i];
			std::string str(std::to_string(i));
			str.append(": (");
			for (auto& pass : passesTask)
			{
				str.append(pass->name);
				if (pass != *(passesTask.end() - 1))
					str.append(", ");
			}
			str.append(")");
			spdlog::info("{}", str);
		}
		spdlog::info("]");
	}

	void RenderGraph::_allocResources()
	{
		for (auto& _passesTask : _passesTasks)
		{
			for (auto& pass : _passesTask)
			{
				for (auto resEdge : pass->outputs)
				{
					if (_registry._buffersMap.contains(resEdge->handle))
					{
						auto& [bufferDesc, buffer] = _registry._buffersMap[resEdge->handle];
						if (buffer)	continue;

						buffer = _device->createBuffer(bufferDesc);
						spdlog::debug("render graph {} create buffer {}", name, resEdge->name);
					}
				}
			}
		}
	}

	void RenderGraph::_allocMemory()
	{
		size_t memOffset = 0;
		for (uint32_t taskIndex = 0; taskIndex < _passesTasks.size(); ++taskIndex)
		{
			std::list<std::pair<size_t, size_t>> retireMemBlocks; // offset, size
			for (auto& pair : _registry._texturesMap)
			{
				auto resEdge = _getResourceEdge(pair.first);
				if (resEdge->lifecycle.lastUseTask < taskIndex)
				{
					auto& [textureDesc, size, _, __] = _registry._texturesMap[resEdge->handle];
					retireMemBlocks.emplace_back(std::make_pair(memOffset, size));
				}
			}

			for (auto& pass : _passesTasks[taskIndex])
			{
				for (auto resEdge : pass->outputs)
				{
					if (_registry._texturesMap.contains(resEdge->handle))
					{
						auto& [textureDesc, size, _, __] = _registry._texturesMap[resEdge->handle];
						auto it = std::find_if(retireMemBlocks.begin(), retireMemBlocks.end(), [&textureDesc](auto& retireMemBlock)
							{
								return retireMemBlock.second >= textureDesc.width * textureDesc.height;
							});
						if (it == retireMemBlocks.end())
						{
							size = _device->getTextureAllocateSize(textureDesc);
							memOffset += size;
						}
						else
						{
							retireMemBlocks.erase(it);
						}
					}
				}
			}
		}

		_memory = _device->createMemory({ .size = memOffset });
		spdlog::debug("render graph {} alloc memory size {}", name, memOffset);
	}

	void RenderGraph::_createResources()
	{
		size_t memOffset = 0;
		for (uint32_t taskIndex = 0; taskIndex < _passesTasks.size(); ++taskIndex)
		{
			std::list<std::pair<size_t, size_t>> retireMemBlocks; // offset, size
			for (auto& pair : _registry._texturesMap)
			{
				auto resEdge = _getResourceEdge(pair.first);
				if (resEdge->lifecycle.lastUseTask < taskIndex)
				{
					auto& [textureDesc, size, _, __] = _registry._texturesMap[resEdge->handle];
					retireMemBlocks.emplace_back(std::make_pair(memOffset, size));
				}
			}
			
			for (auto& pass : _passesTasks[taskIndex])
			{
				for (auto resEdge : pass->outputs)
				{
					if (_registry._texturesMap.contains(resEdge->handle))
					{
						auto& [textureDesc, size, texture, textureView] = _registry._texturesMap[resEdge->handle];
						assert(!texture);

						size_t offset = 0;
						std::string useMemoryAliasing;
						auto it = std::find_if(retireMemBlocks.begin(), retireMemBlocks.end(), [&textureDesc](auto& retireMemBlock)
							{
								return retireMemBlock.second >= textureDesc.width * textureDesc.height;
							});
						if (it == retireMemBlocks.end())
						{
							offset = memOffset;
							memOffset += size;
						}
						else
						{
							retireMemBlocks.erase(it);
							useMemoryAliasing = ". use memory aliasing";
						}

						texture = _device->createTexture(textureDesc, _memory, offset);
						textureView = texture->createView({});
						spdlog::debug("render graph {} create texture {}{}", name, resEdge->name, useMemoryAliasing);
					}
				}
			}
		}
	}

	inline static bool isDepthFormat(Format format)
	{
		return format == Format::D16Unorm ||
			format == Format::D32Sfloat ||
			format == Format::D24UnormS8Uint ||
			format == Format::D32SfloatS8Uint;
	}
	void RenderGraph::_autoSetTextureUsage()
	{
		for (auto& texturePair : _registry._texturesMap)
		{
			auto resEdge = _getResourceEdge(texturePair.first);
			auto& [textureDesc, _, texture, __] = texturePair.second;
			if (texture)	continue;

			for (const auto& consumer : resEdge->consumers)
			{
				if (consumer->type == RenderGraphPassType::Copy)
					textureDesc.usage |= TextureUsage::CopySrc;
				else if (resEdge->isUAV)
					textureDesc.usage |= TextureUsage::Storage;
				else
					textureDesc.usage |= TextureUsage::Sampled;
			}

			if (resEdge->producer->type == RenderGraphPassType::Raster ||
				resEdge->producer->type == RenderGraphPassType::FrameRaster)
			{
				if (isDepthFormat(textureDesc.format))
					textureDesc.usage |= TextureUsage::DepthStencilAttachment;
				else
					textureDesc.usage |= TextureUsage::ColorAttachment;
			}
			if (resEdge->producer->type == RenderGraphPassType::Copy)
			{
				textureDesc.usage |= TextureUsage::CopyDst;
			}
		}
	}

	void RenderGraph::_autoTransitionTextureState(RenderGraphPassNode* pass, std::shared_ptr<CommandList> commandList)
	{
		for (auto inputResEdge : pass->inputs)
		{
			const auto& [texture, textureView] = _registry.getTexture(inputResEdge->handle);
			if (texture)
			{
				TextureState newState = TextureState::ShaderRead;
				if (isDepthFormat(texture->getFormat()))
				{
					newState = TextureState::DepthStencilRead;
				}
				if (pass->type == RenderGraphPassType::Compute || pass->type == RenderGraphPassType::FrameCompute)
				{
					newState = TextureState::General;
				}
				else if (pass->type == RenderGraphPassType::Copy)
				{
					newState = TextureState::CopySrc;
				}
				commandList->resourceBarrier({ texture, texture->getState(), newState });
			}
		}
		for (auto outputResEdge : pass->outputs)
		{
			const auto& [texture, textureView] = _registry.getTexture(outputResEdge->handle);
			if (texture)
			{
				TextureState newState = TextureState::ColorAttachment;
				if (isDepthFormat(texture->getFormat()))
				{
					newState = TextureState::DepthStencilWrite;
				}
				if (pass->type == RenderGraphPassType::Compute || pass->type == RenderGraphPassType::FrameCompute)
				{
					newState = TextureState::General;
				}
				else if (pass->type == RenderGraphPassType::Copy)
				{
					newState = TextureState::CopyDst;
				}
				commandList->resourceBarrier({ texture, texture->getState(), newState });
			}
		}
	}

	void RenderGraph::genDotFile(const std::string& filename, bool showImportRes)
	{
		std::string content;
		content.append(fmt::format("digraph {} {{\n", name));
		content.append("	rankdir=LR\n");
		content.append("	node [style=filled, color=black]\n\n");

		// 外部导入资源
		std::unordered_set<RenderGraphResourceEdge*> outerResourceEdges;

		// 定义Pass节点
		for (uint32_t i = 0; i < _passes.size(); i++)
		{
			auto pass = _passes[i];
			content.append(fmt::format("	pass{} [", i));

			std::string fillcolor = "dodgerblue";
			if (pass->type == RenderGraphPassType::Raster || pass->type == RenderGraphPassType::FrameRaster)
			{
				fillcolor = "slateblue";
			}
			else if (pass->type == RenderGraphPassType::Compute || pass->type == RenderGraphPassType::FrameCompute)
			{
				fillcolor = "royalblue";
			}
			content.append(fmt::format("label=\"{}\", fillcolor = {}", pass->name, fillcolor));

			if (pass->isCullBase)
			{
				content.append(", color=red");
			}

			content.append("];\n");

			// 外部资源
			for (auto input : pass->inputs)
			{
				if (!input->producer)
					outerResourceEdges.insert(input);
			}
			for (auto output : pass->outputs)
			{
				if (output->consumers.empty())
					outerResourceEdges.insert(output);
			}
		}
		content.append("\n");

		// 定义外部资源节点
		if (showImportRes)
		{
			for (auto resEdge : outerResourceEdges)
			{
				content.append(fmt::format("	res{} [", resEdge->handle));
				content.append(fmt::format("shape=box, label=\"{}\", fillcolor = tomato", resEdge->name));
				content.append("];\n");
			}
		}

		// 定义边
		for (uint32_t i = 0; i < _passes.size(); i++)
		{
			auto pass = _passes[i];
			for (auto input : pass->inputs)
			{
				if (!input->producer && showImportRes)
				{
					content.append(fmt::format("	res{} -> pass{};\n", input->handle, i));
				}
			}
			for (auto output : pass->outputs)
			{
				for (auto consumerPass : output->consumers)
				{
					auto it = std::find(_passes.begin(), _passes.end(), consumerPass);
					uint32_t outIndex = std::distance(_passes.begin(), it);
					content.append(fmt::format("	pass{} -> pass{} [label=\"{}\"];\n", i, outIndex, output->name));
				}
				if (output->consumers.empty() && showImportRes)
				{
					content.append(fmt::format("	pass{} -> res{};\n", i, output->handle));
				}
			}
		}
		content.append("\n");

		content.append("}");
		WriteTextFile(filename, content);
	}

	void RenderGraph::drawImNodes(float dpiScale)
	{
		const float xPadding = 100.f * dpiScale;
		const float yPadding = 100.f * dpiScale;
		const float xSpace = 200.f * dpiScale;
		const float ySpace = 200.f * dpiScale;

		if (_imNodesDirty)
		{
			uint32_t maxY = 0;
			for (auto& task : _passesTasks)	maxY = std::max(maxY, (uint32_t)task.size());
			float sizeX = xPadding * 2.f + xSpace * (_passesTasks.size() - 1);
			float sizeY = yPadding * 2.f + ySpace * (maxY - 1);
			ImGui::SetNextWindowSize(ImVec2(sizeX, sizeY));
			ImGui::SetNextWindowCollapsed(true);
		}
		
		ImGui::Begin("Render Graph");
		ImNodes::BeginNodeEditor();

		int linkIDs = 0;
		std::map<std::pair<int, int>, int> linkIDMap;
		for (int i = _passesTasks.size() - 1; i > -1; --i)
		{
			for (int j = _passesTasks[i].size() - 1; j > -1; --j)
			{
				auto& pass = _passesTasks[i][j];

				const int nodeID = (int)std::distance(std::find(_passes.begin(), _passes.end(), pass), _passes.end());
				ImNodes::BeginNode(nodeID);
				ImNodes::BeginNodeTitleBar();
				ImGui::TextUnformatted(pass->name.c_str());
				ImNodes::EndNodeTitleBar();

				ImGui::Checkbox("Cull Base", &pass->isCullBase);

				int k = 0;
				for (auto& input : pass->inputs)
				{
					if (!input->producer)
					{
						ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(34, 139, 34, 255));
						ImNodes::PushColorStyle(ImNodesCol_PinHovered, IM_COL32(47, 195, 47, 255));
					}
					ImNodes::BeginInputAttribute(++linkIDs);
					ImGui::TextUnformatted(input->name.c_str());
					ImNodes::EndInputAttribute();
					if (!input->producer)
					{
						ImNodes::PopColorStyle();
						ImNodes::PopColorStyle();
					}
					linkIDMap[std::make_pair(nodeID, k++)] = linkIDs;
				}
				for (auto& output : pass->outputs)
				{
					if (output->consumers.empty())
					{
						ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(34, 139, 34, 255));
						ImNodes::PushColorStyle(ImNodesCol_PinHovered, IM_COL32(47, 195, 47, 255));
					}
					ImNodes::BeginOutputAttribute(++linkIDs);
					ImGui::Indent(50.f * dpiScale);
					ImGui::TextUnformatted(output->name.c_str());
					ImNodes::EndOutputAttribute();
					if (output->consumers.empty())
					{
						ImNodes::PopColorStyle();
						ImNodes::PopColorStyle();
					}
					linkIDMap[std::make_pair(nodeID, k++)] = linkIDs;
				}
				ImNodes::EndNode();

				if (_imNodesDirty)
				{
					float posX = i == 0 ? xPadding : xPadding + xSpace * i;
					float posY = j == 0 ? yPadding : yPadding + ySpace * j;
					ImNodes::SetNodeScreenSpacePos(nodeID, ImVec2(posX, posY));
				}
			}
		}

		static std::vector<std::pair<int, int>> links;
		if (_imNodesDirty)
		{
			for (auto consumer : _passes)
			{
				int consumerID = (int)std::distance(std::find(_passes.begin(), _passes.end(), consumer), _passes.end());
				for (int i = 0; i < consumer->inputs.size(); i++)
				{
					int endID = linkIDMap.at({ consumerID, i });
					auto input = consumer->inputs[i];
					if (input->producer)
					{
						int k = input->producer->inputs.size();
						for (auto& output : input->producer->outputs)
						{
							if (input == output) break;
							k++;
						}
						int producerID = (int)std::distance(std::find(_passes.begin(), _passes.end(), input->producer), _passes.end());
						int startID = linkIDMap.at({ producerID, k });
						links.emplace_back(std::make_pair(startID, endID));
					}
				}
			}
		}
		for (int i = 0; i < links.size(); ++i)
		{
			const auto& pair = links[i];
			ImNodes::Link(i, pair.first, pair.second);
		}

		ImNodes::EndNodeEditor();
		ImGui::End();

		{
			int startID, endID;
			if (ImNodes::IsLinkCreated(&startID, &endID))
			{
				links.emplace_back(std::make_pair(startID, endID));
			}
		}
		{
			int linkID;
			if (ImNodes::IsLinkDestroyed(&linkID))
			{
				links.erase(links.begin() + linkID);
			}
		}

		_imNodesDirty = false;
	}
}
