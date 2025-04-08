#pragma once

#include "BaseTypes.h"

namespace kdGfx
{
    // 保存跨Pass数据
    class RenderGraphScope
    {
    public:
        template<typename T>
        inline void add(const std::string& key, const T& data)
        {
            _storage[key] = data;
        }
        template<typename T>
        const T& get(const std::string& key)
        {
            assert(_storage.contains(key));
            return std::any_cast<const T&>(_storage[key]);
        }

        template<typename T>
        void add(const T& data)
        {
            size_t key = typeid(T).hash_code();
            _type_storage[key] = data;
        }
        template<typename T>
        const T& get()
        {
            size_t key = typeid(T).hash_code();
            assert(_type_storage.contains(key));
            return std::any_cast<const T&>(_type_storage[key]);
        }

    private:
        std::unordered_map<std::string, std::any> _storage;
        std::unordered_map<size_t, std::any> _type_storage;
    };
}
