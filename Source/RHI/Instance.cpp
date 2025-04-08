#include "Instance.h"
#include "Vulkan/VKInstance.h"

#ifdef _WIN32
#include "DirectX12/DXInstance.h"
#endif

namespace kdGfx
{
    static Instance* gInstance = nullptr;

    std::shared_ptr<Instance> CreateInstance(BackendType backend, bool debug)
    {
        switch (backend)
        {
        case BackendType::Vulkan:
        {
            spdlog::info("use Vulkan backend");
            std::shared_ptr<Instance> instance = std::make_shared<VKInstance>(debug);
            gInstance = instance.get();
            return instance;
        }
#ifdef _WIN32
		case BackendType::DirectX12:
        {
            spdlog::info("use DirectX12 backend");
            std::shared_ptr<Instance> instance = std::make_shared<DXInstance>(debug);
            gInstance = instance.get();
            return instance;
        }
#endif
        default:
            return nullptr;
        }
    }
}
