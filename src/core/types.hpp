#pragma once

#include <grf/types.hpp>

using i8  = grf::i8;
using i16 = grf::i16;
using i32 = grf::i32;
using i64 = grf::i64;
using u8  = grf::u8;
using u16 = grf::u16;
using u32 = grf::u32;
using u64 = grf::u64;

using f32 = grf::f32;
using f64 = grf::f64;

using str = grf::str;

using vec2  = grf::vec2;
using vec3  = grf::vec3;
using vec4  = grf::vec4;
using ivec2 = grf::ivec2;
using ivec3 = grf::ivec3;
using ivec4 = grf::ivec4;
using uvec2 = grf::uvec2;
using uvec3 = grf::uvec3;
using uvec4 = grf::uvec4;
using dvec2 = grf::dvec2;
using dvec3 = grf::dvec3;
using dvec4 = grf::dvec4;

using mat2 = grf::mat2;
using mat3 = grf::mat3;
using mat4 = grf::mat4;

using quat = grf::quat;

template <typename T>
using Ring = grf::Ring<T>;

using byte = std::byte;
using size = std::size_t;
