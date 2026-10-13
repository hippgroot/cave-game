#pragma once

#include "net/protocol.hpp"

#include <bit>
#include <concepts>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace net {

inline constexpr size maxStringLength = 4096;

class Writer {
  std::vector<u8> m_bytes;

public:
  template <std::integral T>
  void write(T value) {
    using U     = std::make_unsigned_t<T>;
    const U raw = static_cast<U>(value);
    for (size i = 0; i < sizeof(T); ++i)
      m_bytes.push_back(static_cast<u8>(raw >> (8 * i)));
  }

  void write(f32 value) { write(std::bit_cast<u32>(value)); }
  void write(f64 value) { write(std::bit_cast<u64>(value)); }

  void write(const str& value) {
    write(static_cast<u32>(value.size()));
    m_bytes.insert(m_bytes.end(), value.begin(), value.end());
  }

  template <glm::length_t L, typename T, glm::qualifier Q>
  void write(const glm::vec<L, T, Q>& value) {
    for (glm::length_t i = 0; i < L; ++i)
      write(value[i]);
  }

  std::vector<u8> take() { return std::move(m_bytes); }
};

class Reader {
  std::span<const u8> m_bytes;
  size                m_offset = 0;
  bool                m_ok     = true;

public:
  explicit Reader(std::span<const u8> bytes)
  : m_bytes(bytes) {}

  template <std::integral T>
  void read(T& value) {
    if (!take(sizeof(T))) return;

    using U = std::make_unsigned_t<T>;
    U raw   = 0;
    for (size i = 0; i < sizeof(T); ++i)
      raw |= static_cast<U>(static_cast<U>(m_bytes[m_offset - sizeof(T) + i]) << (8 * i));
    value = static_cast<T>(raw);
  }

  void read(f32& value) {
    u32 raw = 0;
    read(raw);
    value = std::bit_cast<f32>(raw);
  }

  void read(f64& value) {
    u64 raw = 0;
    read(raw);
    value = std::bit_cast<f64>(raw);
  }

  void read(str& value) {
    u32 length = 0;
    read(length);
    if (!m_ok || length > maxStringLength || !take(length)) {
      m_ok = false;
      return;
    }
    const auto* begin = reinterpret_cast<const char*>(m_bytes.data() + m_offset - length);
    value.assign(begin, length);
  }

  template <glm::length_t L, typename T, glm::qualifier Q>
  void read(glm::vec<L, T, Q>& value) {
    for (glm::length_t i = 0; i < L; ++i)
      read(value[i]);
  }

  bool ok() const { return m_ok; }
  bool finished() const { return m_ok && m_offset == m_bytes.size(); }

private:
  bool take(size count) {
    if (!m_ok || m_bytes.size() - m_offset < count) {
      m_ok = false;
      return false;
    }
    m_offset += count;
    return true;
  }
};

namespace detail {

template <typename Variant, size... Is>
std::optional<Variant> makeAlternative(size index, std::index_sequence<Is...>) {
  std::optional<Variant> result;
  static_cast<void>(((index == Is ? (result.emplace(std::in_place_index<Is>), true) : false) || ...));
  return result;
}

}

template <typename Message>
std::vector<u8> encode(const Message& message) {
  Writer writer;
  writer.write(static_cast<u8>(message.index()));
  std::visit([&writer](const auto& alternative) {
    std::apply([&writer](const auto&... field) { (writer.write(field), ...); }, alternative.fields());
  },
    message);
  return writer.take();
}

template <typename Message>
std::optional<Message> decode(std::span<const u8> bytes) {
  Reader reader(bytes);

  u8 index = 0;
  reader.read(index);
  if (!reader.ok() || index >= std::variant_size_v<Message>) return std::nullopt;

  auto message = detail::makeAlternative<Message>(index, std::make_index_sequence<std::variant_size_v<Message>>{});
  std::visit([&reader](auto& alternative) {
    std::apply([&reader](auto&... field) { (reader.read(field), ...); }, alternative.fields());
  },
    *message);

  if (!reader.finished()) return std::nullopt;
  return message;
}

}
