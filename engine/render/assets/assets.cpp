#include "./assets.hpp"

#include "grf/device.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

using namespace grf;
namespace fs = std::filesystem;

Assets::Assets(const Device& device, Format swapchainFormat, Format depthFormat) : m_rootDir(ASSETS_DIR) {
  loadTextures(device);
  createSamplers(device);
  compileShaders(device);
  createPipelines(device, swapchainFormat, depthFormat);
}

const Asset<Image>& Assets::textures() const {
  return m_textures;
}

const Asset<Sampler>& Assets::samplers() const {
  return m_samplers;
}

const Asset<Shader>& Assets::shaders() const {
  return m_shaders;
}

const Asset<Pipeline>& Assets::pipelines() const {
  return m_pipelines;
}

Buffer Assets::textureLUT() const {
  return m_textureLUT;
}

void Assets::loadTextures(const Device& device) {
  logging::general("Loading textures...");

  std::vector<fs::path> paths;
  for (const auto& entry : fs::directory_iterator(m_rootDir/"textures"))
    paths.emplace_back(entry.path());

  std::stable_sort(paths.begin(), paths.end(), [](const fs::path& a, const fs::path& b){
    std::stringstream ssa(a.stem());
    std::stringstream ssb(b.stem());

    i32 numa, numb;
    ssa >> numa;
    ssb >> numb;

    return numa < numb;
  });

  std::array<u32, 256> lut = { 255 };
  for (u32 i = 0; const fs::path& path : paths) {
    std::stringstream ss(path.stem().string());
    ss.ignore(4, '_');

    str name;
    ss >> name;

    Image::Data data = Image::getDataFromFile(path);

    Image image = device.createImage(Image::Info{
      .width  = data.width,
      .height = data.height,
      .format = data.format,
      .usage  = ImageUsage::Sampled
    });
    image.write(data.bytes, Layout::ShaderReadOptimal);

    lut[++i] = image.sampledHeapIndex();

    m_textures.add(name, image);
    logging::general("- {}", name);
  }

  logging::general("Creating texture LUT...");

  m_textureLUT = device.createBuffer(Buffer::Info{
    .size   = static_cast<u32>(sizeof(u32) * lut.size()),
    .intent = BufferIntent::GPUOnly
  });
  m_textureLUT.write(lut);
}

void Assets::createSamplers(const Device& device) {
  const auto createSampler = [this, &device](const str& name, const Sampler::Info& info) {
    m_samplers.add(name, device.createSampler(std::move(info)));
    logging::general("- {}", name);
  };

  logging::general("Creating samplers...");

  createSampler("nearest_clamp", Sampler::Info{
    .magFilter    = Filter::Nearest,
    .minFilter    = Filter::Nearest,
    .addressModeU = AddressMode::ClampToBorder,
    .addressModeV = AddressMode::ClampToBorder,
    .addressModeW = AddressMode::ClampToBorder
  });
}

void Assets::compileShaders(const Device& device) {
  const fs::path _dir = m_rootDir/"shaders";
  const auto compile = [this, &device](ShaderStage stage, const fs::path& path){
    str name = path.filename();
    m_shaders.add(name, device.compileGlslShader(stage, path));
    logging::general("- {}", name);
  };

  compile(ShaderStage::Vertex, _dir/"chunk_draw"/"chunk.vert");
  compile(ShaderStage::Fragment, _dir/"chunk_draw"/"chunk.frag");
}

void Assets::createPipelines(const Device& device, Format swapchainFormat, Format depthFormat) {
  const auto createGraphics = [this, &device, &swapchainFormat](
    const str& name,
    std::initializer_list<Shader> shaders,
    GraphicsPipelineSettings settings
  )
  {
    m_pipelines.add(
      name,
      device.createGraphicsPipeline(std::span<const Shader>(shaders.begin(), shaders.size()), settings)
    );
    logging::general("- {}", name);
  };

  const auto createCompute = [this, &device](const str& name, Shader shader) {
    m_pipelines.add(name, device.createComputePipeline(shader));
    logging::general("- {}", name);
  };

  auto& s = shaders();

  logging::general("Creating pipelines...");

  createGraphics("chunk_draw", { s["chunk.vert"], s["chunk.frag"] },
    GraphicsPipelineSettings{
      .colorFormats = { swapchainFormat },
      .depthFormat  = depthFormat,
      .depthTest    = true,
      .depthWrite   = true,
      .blends = {
        BlendState{
          .enable         = true,
          .srcColorFactor = BlendFactor::SrcAlpha,
          .dstColorFactor = BlendFactor::OneMinusSrcAlpha
        }
      }
    }
  );
}