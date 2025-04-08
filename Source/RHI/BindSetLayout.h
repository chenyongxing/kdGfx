#pragma once

#include "BaseTypes.h"

namespace kdGfx
{
    class BindSetLayout
    {
    public:
        virtual ~BindSetLayout() = default;

        inline const std::vector<BindEntryLayout>& getEntryLayouts() const { return _entryLayouts; }

    protected:
        std::vector<BindEntryLayout> _entryLayouts;
    };
}
