#pragma once

#include "types.hpp"

#include "grf/resources.hpp"

namespace grf {

class Device;
class Pipeline;
class Shader;

}

template <typename T>
class Asset {
  std::vector<T>                        m_assets;
  std::unordered_map<str, std::size_t>  m_indices;
  std::unordered_map<std::size_t, str>  m_names;

public:
  Asset() = default;
  ~Asset() = default;

  Asset(const Asset&) = default;
  Asset(Asset&&) = default;
  Asset& operator=(const Asset&) = default;
  Asset& operator=(Asset&&) = default;

  T operator[](u32 index) const {
    return m_assets[index];
  }

  T operator[](const str& name) const {
    return m_assets[m_indices.at(name)];
  }

  void add(const str& name, T asset) {
    std::size_t index = m_assets.size();

    m_indices.emplace(name, index);
    m_names.emplace(index, name);

    m_assets.push_back(std::move(asset));
  }

  u32 retrieveIndex(const str& name) const {
    return m_indices.at(name);
  }

  const str& retrieveName(u32 index) const {
    return m_names.at(index);
  }
};

class Assets {
  const std::filesystem::path m_rootDir;

  Asset<grf::Image>     m_textures;
  Asset<grf::Sampler>   m_samplers;
  Asset<grf::Shader>    m_shaders;
  Asset<grf::Pipeline>  m_pipelines;

  grf::Buffer           m_textureLUT;

public:
  Assets(const grf::Device& device, grf::Format swapchainFormat, grf::Format depthFormat);
  ~Assets() = default;

  Assets(const Assets&) = delete;
  Assets(Assets&&) = delete;
  Assets& operator=(const Assets&) = delete;
  Assets& operator=(Assets&&) = delete;

  const Asset<grf::Image>& textures() const;
  const Asset<grf::Sampler>& samplers() const;
  const Asset<grf::Shader>& shaders() const;
  const Asset<grf::Pipeline>& pipelines() const;

  grf::Buffer textureLUT() const;

private:
  void loadTextures(const grf::Device& device);
  void createSamplers(const grf::Device& device);
  void compileShaders(const grf::Device& device);
  void createPipelines(const grf::Device& device, grf::Format swapchainFormat, grf::Format depthFormat);
};