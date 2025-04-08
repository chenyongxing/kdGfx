#pragma once

#include <limits>
#include <typeinfo>
#include <tuple>
#include <any>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <iostream>
#include <filesystem>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/ext/matrix_relational.hpp>

#include "MathLib.h"

#define ENUM_BITWISE_OPERATOR(EnumType) \
inline EnumType operator |(EnumType lhs, EnumType rhs) \
{ \
    return static_cast<EnumType>(static_cast<int>(lhs) | static_cast<int>(rhs)); \
} \
inline int operator &(EnumType lhs, EnumType rhs) \
{ \
    return static_cast<int>(lhs) & static_cast<int>(rhs); \
} \
inline void operator |=(EnumType& lhs, EnumType rhs) \
{ \
    lhs = static_cast<EnumType>(static_cast<int>(lhs) | static_cast<int>(rhs)); \
}

template<typename T, typename U>
constexpr bool HasAllBits(T flags, U bits) noexcept
{
    return (flags & bits) == bits;
}

template<typename T, typename U>
constexpr auto HasAnyBits(T flags, U bits) noexcept
{
    return (flags & bits) != 0;
}

template <typename T, std::size_t N>
constexpr std::size_t ArraySize(T(&)[N]) noexcept
{
    return N;
}

template <typename T1, typename T2>
constexpr auto MemAlign(T1 size, T2 align) noexcept
{
    using CommonType = std::common_type_t<T1, T2>;
    return (static_cast<CommonType>(size) + (static_cast<CommonType>(align) - 1)) & ~(static_cast<CommonType>(align) - 1);
}

template <typename T>
void HashCombine(size_t& seed, const T& value)
{
    seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline bool EqualNearly(float a, float b, float epsilon = 0.0001f)
{
    if (fabs(a - b) > epsilon)	return false;
    return true;
}

inline bool EqualNearly(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
{
    if (fabs(a.x - b.x) > epsilon)	return false;
    if (fabs(a.y - b.y) > epsilon)	return false;
    if (fabs(a.z - b.z) > epsilon)	return false;
    return true;
}

inline bool EqualNearly(glm::mat4 const& a, glm::mat4 const& b, float epsilon = 0.0001f)
{
    glm::bvec4 const equals = glm::equal(a, b, epsilon);
    return glm::all(equals);
}

#ifdef _WIN32
#include <Windows.h>
inline void RuntimeError(const std::string& msg, bool bExit = true)
{
    MessageBoxA(NULL, msg.c_str(), "Error", MB_ICONERROR | MB_OK);
    if (bExit)	exit(1);
}
#else
inline void RuntimeError(const std::string& msg, bool bExit = true)
{
    throw std::runtime_error(msg);
    if (bExit)	exit(1);
}
#endif // _WIN32
