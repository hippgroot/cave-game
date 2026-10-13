#include "engine/assets.hpp"

#include <grf/device.hpp>
#include <yaml-cpp/yaml.h>

#include <array>
#include <format>
#include <optional>
#include <stdexcept>

using namespace grf;
namespace fs = std::filesystem;

namespace {

std::optional<ShaderStage> stageFromExtension(const fs::path& extension) {
  if (extension == ".vert") return ShaderStage::Vertex;
  if (extension == ".frag") return ShaderStage::Fragment;
  if (extension == ".comp") return ShaderStage::Compute;
  return std::nullopt;
}

}

Assets::Assets(const Device& device)
: m_rootDir(ASSETS_DIR) {
  loadBlockTextures(device);
  compileShaders(device);
}

const Registry<Shader>& Assets::shaders() const {
  return m_shaders;
}

Buffer Assets::textureLUT() const {
  return m_textureLUT;
}

void Assets::loadBlockTextures(const Device& device) {
  const YAML::Node manifest = YAML::LoadFile((m_rootDir / "blocks.yaml").string());

  std::array<u32, maxBlocks> lut;
  lut.fill(loadTexture(device, manifest["fallback"].as<str>()));

  for (const YAML::Node& block : manifest["blocks"]) {
    u32 id = block["id"].as<u32>();
    if (id >= maxBlocks)
      throw std::runtime_error(std::format("Block '{}' has id {} (max {})", block["name"].as<str>(), id, maxBlocks - 1));

    if (const YAML::Node texture = block["texture"])
      lut[id] = loadTexture(device, texture.as<str>());
  }

  m_textureLUT = device.createBuffer(Buffer::Info{
    .size   = static_cast<u32>(sizeof(u32) * lut.size()),
    .intent = BufferIntent::GPUOnly
  });
  m_textureLUT.write(lut);
}

void Assets::compileShaders(const Device& device) {
  const fs::path dir = m_rootDir / "shaders";

  for (const auto& entry : fs::recursive_directory_iterator(dir)) {
    if (!entry.is_regular_file()) continue;

    auto stage = stageFromExtension(entry.path().extension());
    if (!stage) continue;

    m_shaders.add(
      fs::relative(entry.path(), dir).generic_string(),
      device.compileGlslShader(*stage, entry.path()));
  }
}

u32 Assets::loadTexture(const Device& device, const str& path) {
  if (!m_textures.contains(path)) {
    Image::Data data = Image::getDataFromFile(m_rootDir / path);

    Image image = device.createImage(Image::Info{
      .width  = data.width,
      .height = data.height,
      .format = data.format,
      .usage  = ImageUsage::Sampled
    });
    image.write(data.bytes, Layout::ShaderReadOptimal);

    m_textures.add(path, std::move(image));
  }

  return m_textures[path].sampledHeapIndex();
}
