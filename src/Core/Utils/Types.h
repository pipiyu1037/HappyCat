#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace happycat {

// Basic types
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

// Size type
using Size = size_t;

// Handle types
using ResourceHandle = u64;
constexpr ResourceHandle INVALID_HANDLE = 0;

// Smart pointers
template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename... Args>
Scope<T> CreateScope(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
Ref<T> CreateRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// Array and String
template<typename T>
using Array = std::vector<T>;

using String = std::string;

template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

// Function
template<typename T>
using Function = std::function<T>;

} // namespace happycat
