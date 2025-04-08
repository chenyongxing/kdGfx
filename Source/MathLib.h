#pragma once

#include "PCH.h"

template<class T>
constexpr const T& Clamp(const T& v, const T& lo, const T& hi)
{
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

template<class T>
constexpr T DegreeToRadian(T degree)
{
    return degree * 3.14159265358979323846 / 180.0;
}

template<class T>
constexpr T RadianToDegree(T radian)
{
    return radian * 180.0 / 3.14159265358979323846;
}

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

struct Vector2
{
    float x = 0.f;
    float y = 0.f;

    Vector2() = default;
    Vector2(float x) : x(x), y(x) {}
    Vector2(float x, float y) : x(x), y(y) {}

    inline float& operator[](int i)
    {
        if (i == 0) return x;
        return y;
    }

    inline Vector2 operator+(Vector2 v) const
    {
        return Vector2(x + v.x, y + v.y);
    }

    inline Vector2 operator-(Vector2 v) const
    {
        return Vector2(x - v.x, y - v.y);
    }

    inline Vector2 operator*(Vector2 v) const
    {
        return Vector2(x * v.x, y * v.y);
    }

    inline Vector2 operator/(Vector2 v) const
    {
        return Vector2(x / v.x, y / v.y);
    }

    template <typename T>
    Vector2 operator*(T s) const
    {
        return Vector2(x * s, y * s);
    }

    template <typename T>
    Vector2 operator/(T f) const
    {
        float inv = (float)(1.0f / f);
        return Vector2(x * inv, y * inv);
    }
};

inline bool operator==(const Vector2& v1, const Vector2& v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

inline bool operator!=(const Vector2& v1, const Vector2& v2)
{
    return v1.x != v2.x || v1.y != v2.y;
}

inline float Length(const Vector2& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline Vector2 Normalize(const Vector2& v)
{
    return v / Length(v);
}

inline float Dot(const Vector2& v1, const Vector2& v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

inline float Cross(const Vector2& v1, const Vector2& v2)
{
    return v1.x * v2.y - v1.y * v2.x;
}

inline Vector2 Perpendicular(const Vector2& v)
{
    return Vector2(v.y, -v.x);
}

inline Vector2 Min(const Vector2& p1, const Vector2& p2)
{
    return Vector2(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
}

inline Vector2 Max(const Vector2& p1, const Vector2& p2)
{
    return Vector2(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
}

inline std::ostream& operator<<(std::ostream& os, const Vector2& v)
{
    os << "[ " << v.x << ", " << v.y << " ]";
    return os;
}

inline bool EqualNearly(const Vector2& a, const Vector2& b, float epsilon = 0.0001f)
{
    if (fabs(a.x - b.x) > epsilon)	return false;
    if (fabs(a.y - b.y) > epsilon)	return false;
    return true;
}

struct Matrix3x2
{
    float a = 1.f;
	float b = 0.f;
	float c = 0.f;
	float d = 1.f;
	float tx = 0.f;
	float ty = 0.f;

    inline void identity()
    {
        a = 1.f;
        b = 0.f;
        c = 0.f;
        d = 1.f;
        tx = 0.f;
        ty = 0.f;
    }

    inline Vector2 transformPoint(const Vector2 point)
    {
		return Vector2(a * point.x + c * point.y + tx, b * point.x + d * point.y + ty);
    }

    inline Matrix3x2 operator*(const Matrix3x2& rhs)
    {
        return
        {
            .a = this->a * rhs.a + this->b * rhs.c,
            .b = this->a * rhs.b + this->b * rhs.d,
            .c = this->c * rhs.a + this->d * rhs.c,
            .d = this->c * rhs.b + this->d * rhs.d,
            .tx = this->a * rhs.tx + this->c * rhs.ty + this->tx,
            .ty = this->b * rhs.tx + this->d * rhs.ty + this->ty
        };
    }

    inline void translate(float tx, float ty)
    {
		Matrix3x2 translation = { 1.f, 0.f, 0.f, 1.f, tx, ty };
		*this = (*this) * translation;
    }

    inline void scale(float sx, float sy)
    {
        Matrix3x2 scale = { sx, 0.f, 0.f, sy, 0.f, 0.f };
        *this = (*this) * scale;
    }

    inline void rotate(float rad)
    {
        Matrix3x2 rotation = { cosf(rad), sinf(rad), -sinf(rad), cosf(rad), 0.f, 0.f };
        *this = (*this) * rotation;
    }
};

inline Matrix3x2 operator*(const Matrix3x2& lhs, const Matrix3x2& rhs)
{
    return
    {
        .a = lhs.a * rhs.a + lhs.b * rhs.c,
        .b = lhs.a * rhs.b + lhs.b * rhs.d,
        .c = lhs.c * rhs.a + lhs.d * rhs.c,
        .d = lhs.c * rhs.b + lhs.d * rhs.d,
        .tx = lhs.a * rhs.tx + lhs.c * rhs.ty + lhs.tx,
        .ty = lhs.b * rhs.tx + lhs.d * rhs.ty + lhs.ty
    };
}
