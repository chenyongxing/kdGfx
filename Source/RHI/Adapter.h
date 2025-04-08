#pragma once

#include "../PCH.h"
#include "Device.h"

namespace kdGfx
{
    // 对应显卡或者软模拟
    class Adapter
    {
    public:
        virtual ~Adapter() = default;

        virtual std::shared_ptr<Device> createDevice() = 0;

        inline const std::string& getName() const { return _name; }
        inline const bool isDiscreteGPU() const { return _discreteGPU; }

    protected:
        std::string _name;
        bool _discreteGPU = false;
    };
}
