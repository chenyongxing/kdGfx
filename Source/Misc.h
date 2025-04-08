#pragma once

#include "PCH.h"
#include <fstream>
#include <locale>
#include <codecvt>

inline bool LoadBinaryFile(const std::string& filepath, std::vector<char>& result)
{
    std::ifstream ifile(filepath, std::ios::binary | std::ios::ate);
    if (ifile.is_open())
    {
        size_t fileSize = ifile.tellg();
        ifile.seekg(0);
        result.resize(fileSize, '\0');
        ifile.read(result.data(), fileSize);
        ifile.close();
        return true;
    }
    return false;
}

inline bool WriteTextFile(const std::string& filepath, const std::string& content)
{
    std::ofstream ofile(filepath);
    if (ofile.is_open())
    {
        ofile << content;
        ofile.close();
        return true;
    }
    return false;
}

inline std::wstring StringToWString(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.from_bytes(str);
}

inline std::string WStringToString(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
}

struct EventData
{
    virtual ~EventData() = default;
};

class EventTower
{
public:
    using EventCallback = std::function<void(std::shared_ptr<EventData> data)>;

    inline size_t addEventListener(const std::string& name, const EventCallback& callback)
    {
        _eventListeners[name].push_back(std::make_pair(callback, false));
        return _eventListeners[name].size() - 1;
    }

    inline size_t addEventListenerCallOnce(const std::string& name, const EventCallback& callback)
    {
        _eventListeners[name].push_back(std::make_pair(callback, true));
        return _eventListeners[name].size() - 1;
    }

    inline void removeEventListener(const std::string& name, size_t index)
    {
        if (_eventListeners.count(name) > 0)
        {
            auto& listeners = _eventListeners[name];
            listeners.erase(listeners.begin() + index);
        }
    }

    inline void dispatchEvent(const std::string& name, std::shared_ptr<EventData> data = nullptr)
    {
        if (_eventListeners.count(name) > 0)
        {
            auto& listeners = _eventListeners[name];
            for (auto it = listeners.begin(); it != listeners.end(); )
            {
                it->first(data);
                if (it->second)
                {
                    it = listeners.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

private:
    std::unordered_map<std::string, std::vector<std::pair<EventCallback, bool>>> _eventListeners;
};

template<typename T>
class EasingAnimation
{
private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;

public:
    T start;
    T target;
    float duration = 1.0f;
    std::function<float(float)> easeFunction;
    std::function<void()> endCallback;

    bool isRunning = false;
    bool reverse = false;
    T value;

    void run()
    {
        startTime = std::chrono::steady_clock::now();
        isRunning = true;
        reverse = false;
    }

    void runReverse()
    {
        startTime = std::chrono::steady_clock::now();
        isRunning = true;
        reverse = true;
    }

    void stop()
    {
        isRunning = false;
        value = start;
    }

    void frame()
    {
        if (!isRunning) return;

        auto currentTime = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        float t = (elapsedTime / 1000.f) / duration;
        if (t > 1.f)
        {
            isRunning = false;
            if (endCallback)	endCallback();
            return;
        }

        float factor = easeFunction(t);
        if (reverse)
            value = target + factor * (start - target);
        else
            value = start + factor * (target - start); // lerp
    }
};
