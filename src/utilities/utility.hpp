#pragma once
#include "olcPixelGameEngine.h"

namespace utilities
{
    [[nodiscard]] inline olc::vd2d PolarToCartesian(const olc::vd2d polar) noexcept
    {
        return olc::vd2d{polar.x * cos(polar.y), polar.x * sin(polar.y)};
    }

    template <typename T>
    [[nodiscard]] T lerp(const T &v0, const T &v1, float t) noexcept
    {
        return v0 * (1.0f - t) + v1 * t;
    }

    // Map a value val that is between in_start and end_start to the range out_start out_end
    template <class T, class U>
    [[nodiscard]] U map(T in_start, T in_end, U out_start, U out_end, T val) noexcept
    {
        auto t = std::clamp<U>((val - in_start) / (in_end - in_start), 0.0, 1.0);

        return lerp(out_start, out_end, t);
    }

    // If a vector has a magnitude > max, scale it to be max
    template <class V, class T>
    [[nodiscard]] V truncate(V vec, T max) noexcept
    {
        if (vec.mag() > max)
        {
            vec = vec.norm() * max;
        }

        return vec;
    }

    template<typename T>
    T Ease(const T& t)
    {
        return static_cast<T>((std::sin(3.14159265358979323846 * (t - 0.5)) + 1.0) / 2.0);
    }

    template <typename T>
    [[nodiscard]] inline constexpr int signum(T x, std::false_type is_signed)
    {
        return T(0) < x;
    }

    template <typename T>
    [[nodiscard]] inline constexpr int signum(T x, std::true_type is_signed)
    {
        return (T(0) < x) - (x < T(0));
    }

    template <typename T>
    [[nodiscard]] inline constexpr int signum(T x)
    {
        return signum(x, std::is_signed<T>());
    }

    uint16_t posToSector(olc::vf2d position);

    /// @brief Distance from p1 to p2 on a torus
    /// @tparam T distance type
    /// @tparam U limit type
    /// @param p1 Starting point
    /// @param p2 End point
    /// @param limits Torus size where wrapping occurs
    /// @return Distance
    template<typename T = float, typename U = uint16_t>
    olc::v_2d<T> toroidalDistance(olc::v_2d<T> p1, olc::v_2d<T> p2, olc::v_2d<T> limits) {
        olc::v_2d<T> distance = p2 - p1;

        T x_max = static_cast<T>(limits.x);
        while (abs(distance.x) > x_max / 2.0)
        {
            distance.x = distance.x - utilities::signum(distance.x) * x_max;
        }

        T y_max = static_cast<T>(limits.y);
        while (abs(distance.y) > y_max / 2.0)
        {
            distance.y = distance.y - utilities::signum(distance.y) * y_max;
        }

        return distance;
    }

    template<typename T = float>
    olc::v_2d<T> rotate(const olc::v_2d<T>& p, const olc::v_2d<T>& sc) {
        return olc::v_2d<T>{p.x * sc.y - p.y * sc.x, p.x * sc.x + p.y * sc.y};
    }

    olc::Pixel RandomColor();
    olc::Pixel RandomDarkColor();
    olc::Pixel RandomRedColor();
    olc::Pixel RandomGreenColor();
    olc::Pixel RandomBlueColor();
}