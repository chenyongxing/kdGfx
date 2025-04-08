#pragma once

#include "BaseTypes.h"
#include "Adapter.h"

namespace kdGfx
{
	class Instance
	{
	public:
		virtual ~Instance() = default;

		virtual std::vector<std::shared_ptr<Adapter>> enumerateAdapters() = 0;

		inline const BackendType getBackendType() const { return _backend; }

	protected:
		BackendType _backend = BackendType::Vulkan;
	};

	std::shared_ptr<Instance> CreateInstance(BackendType backend, bool debug = false);
}
