#pragma once

#include "PCH.h"

constexpr float Halton(int32_t index, int32_t base)
{
    float f = 1.0f, result = 0.0f;
    while (index > 0)
    {
        f /= float(base);
        result += f * float(index % base);
        index /= base;
    }
    return result;
}

constexpr glm::vec2 Halton23(int32_t index)
{
    return glm::vec2(Halton(index, 2), Halton(index, 3));
}

constexpr std::array<glm::vec2, 8> Halton23Array8()
{
    return 
    {
        Halton23(1),
        Halton23(2),
        Halton23(3),
        Halton23(4),
        Halton23(5),
        Halton23(6),
        Halton23(7),
        Halton23(8)
    };
}
