#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace happycat {

// Vector types
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using IVec2 = glm::ivec2;
using IVec3 = glm::ivec3;
using IVec4 = glm::ivec4;
using UVec2 = glm::uvec2;
using UVec3 = glm::uvec3;
using UVec4 = glm::uvec4;
using DVec2 = glm::dvec2;
using DVec3 = glm::dvec3;
using DVec4 = glm::dvec4;

// Matrix types
using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using DMat2 = glm::dmat2;
using DMat3 = glm::dmat3;
using DMat4 = glm::dmat4;

// Quaternion
using Quat = glm::quat;
using DQuat = glm::dquat;

// Math constants
namespace Math {
    constexpr f32 PI = 3.14159265358979323846f;
    constexpr f32 TWO_PI = 2.0f * PI;
    constexpr f32 HALF_PI = 0.5f * PI;
    constexpr f32 QUARTER_PI = 0.25f * PI;
    constexpr f32 DEG2RAD = PI / 180.0f;
    constexpr f32 RAD2DEG = 180.0f / PI;
    constexpr f32 EPSILON = 1e-6f;

    // Min/Max
    template<typename T>
    constexpr T Min(T a, T b) { return a < b ? a : b; }

    template<typename T>
    constexpr T Max(T a, T b) { return a > b ? a : b; }

    template<typename T>
    constexpr T Clamp(T value, T min, T max) {
        return value < min ? min : (value > max ? max : value);
    }

    // Lerp
    template<typename T>
    constexpr T Lerp(T a, T b, f32 t) {
        return static_cast<T>(a + (b - a) * t);
    }

    // Degrees/Radians conversion
    inline f32 Radians(f32 degrees) { return degrees * DEG2RAD; }
    inline f32 Degrees(f32 radians) { return radians * RAD2DEG; }

    // Alignment
    inline u32 AlignUp(u32 value, u32 alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    inline u64 AlignUp(u64 value, u64 alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    // Check if power of two
    constexpr bool IsPowerOfTwo(u32 value) {
        return value && !(value & (value - 1));
    }

    constexpr bool IsPowerOfTwo(u64 value) {
        return value && !(value & (value - 1));
    }
}

// GLM operator overloads for convenience
using glm::dot;
using glm::cross;
using glm::normalize;
using glm::length;
using glm::distance;
using glm::reflect;
using glm::refract;
using glm::mix;
using glm::step;
using glm::smoothstep;
using glm::perspective;
using glm::ortho;
using glm::lookAt;
using glm::translate;
using glm::rotate;
using glm::scale;
using glm::value_ptr;

} // namespace happycat
