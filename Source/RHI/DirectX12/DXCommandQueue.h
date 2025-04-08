#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../BaseTypes.h"
#include "../CommandQueue.h"

namespace kdGfx
{
    class DXDevice;

    class DXCommandQueue : public CommandQueue
    {
    public:
        DXCommandQueue(DXDevice& device, CommandListType type);

        void signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
        void wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
        void waitIdle() override;
        void submit(const std::vector<std::shared_ptr<CommandList>>& commandLists) override;

        inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> getQueue() const { return _commandQueue; }

    private:
        DXDevice& _device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;
    };
}
