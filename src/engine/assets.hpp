#pragma once

#include "core/types.hpp"

#include <grf/pipeline.hpp>
#include <grf/resources.hpp>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace grf {

class Device;

}

template <typename T>
class Registry {
  std::vector<T>                m_items;
  std::vector<str>              m_names;
  std::unordered_map<str, size> m_indices;

public:
  Registry()  = default;
  ~Registry() = default;

  Registry(const Registry&)            = default;
  Registry(Registry&&)                 = default;
  Registry& operator=(const Registry&) = default;
  Registry& operator=(Registry&&)      = default;

  const T& operator[](u32 index) const {
    return m_items[index];
  }

  const T& operator[](const str& name) const {
    return m_items[m_indices.at(name)];
  }

  void add(const str& name, T item) {
    m_indices.emplace(name, m_items.size());
    m_names.push_back(name);
    m_items.push_back(std::move(item));
  }

  bool contains(const str& name) const {
    return m_indices.contains(name);
  }

  u32 indexOf(const str& name) const {
    return static_cast<u32>(m_indices.at(name));
  }

  const str& nameOf(u32 index) const {
    return m_names[index];
  }
};

class Assets {
public:
  static constexpr u32 maxBlocks = 256u;

private:
  const std::filesystem::path m_rootDir;

  Registry<grf::Image>  m_textures;
  Registry<grf::Shader> m_shaders;
  grf::Buffer           m_textureLUT;

public:
  explicit Assets(const grf::Device& device);
  ~Assets() = default;

  Assets(const Assets&)            = delete;
  Assets(Assets&&)                 = delete;
  Assets& operator=(const Assets&) = delete;
  Assets& operator=(Assets&&)      = delete;

  const Registry<grf::Shader>& shaders() const;
  grf::Buffer                  textureLUT() const;

private:
  void loadBlockTextures(const grf::Device& device);
  void compileShaders(const grf::Device& device);

  u32 loadTexture(const grf::Device& device, const str& path);
};
