#include "assets.hpp"
#include "camera.hpp"
#include "graveyard.hpp"

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

Camera::Camera(f32 fov_deg, uvec2 viewport, vec3 position, vec2 rotation_deg)
: m_position(position),
  m_pitch(std::clamp(glm::radians(rotation_deg.x), -m_pitchLimit, m_pitchLimit)),
  m_yaw(glm::radians(rotation_deg.y)),
  m_fov(glm::radians(fov_deg)),
  m_viewport(viewport)
{}

f32 Camera::farPlane() {
  return m_far;
}

vec3 Camera::position() const {
  return m_position;
}

std::optional<Camera::UpdateData> Camera::update() {
  if (!m_dirty) return std::nullopt;

  quat rotation = glm::angleAxis(m_yaw, m_worldUp) * glm::angleAxis(m_pitch, m_worldRight);

  vec3 forward = rotation * m_worldForward;
  mat4 view = glm::lookAt(m_position, m_position + forward, m_worldUp);

  f32 aspect = static_cast<f32>(m_viewport.x) / static_cast<f32>(m_viewport.y);
  mat4 proj = glm::perspective(m_fov, aspect, m_near, m_far);
  proj[1][1] *= -1;

  mat4 projView = proj * view;

  mat4 invView = glm::translate(mat4(1), m_position) * glm::mat4_cast(rotation);

  f32 t = std::tan(m_fov * 0.5f);
  mat4 invProj(0.0f);
  invProj[0][0] = aspect * t;
  invProj[1][1] = -t;
  invProj[3][2] = -1.0f;
  invProj[2][3] = (m_near - m_far) / (m_far * m_near);
  invProj[3][3] = 1.0f / m_near;

  Frustum frustrum(projView);

  m_dirty = false;
  return std::make_pair(std::move(frustrum), Data{
    .view     = std::move(view),
    .proj     = std::move(proj),
    .projView = std::move(projView),
    .invView  = std::move(invView),
    .invProj  = std::move(invProj),
    .position = m_position,
    .forward  = std::move(forward),
    .viewport = m_viewport,
    .near     = m_near,
    .far      = m_far
  });
}

void Camera::translate(vec3 delta) {
  quat rotation = glm::angleAxis(m_yaw, m_worldUp) * glm::angleAxis(m_pitch, m_worldRight);

  m_position += rotation * vec3(delta.x, 0.0f, delta.z);
  m_position.y += delta.y;

  m_dirty = true;
}

void Camera::rotate(f32 pitch_deg, f32 yaw_deg) {
  m_pitch = glm::clamp(m_pitch + glm::radians(pitch_deg), -m_pitchLimit, m_pitchLimit);
  m_yaw += glm::radians(yaw_deg);
  m_dirty = true;
}

void Camera::setFOV(f32 fov_deg) {
  m_fov = glm::radians(fov_deg);
  m_dirty = true;
}

void Camera::setViewport(u32 width, u32 height) {
  m_viewport = uvec2(width, height);
  m_dirty = true;
}

Camera::Frustum::Frustum(const mat4& projView) {
  auto row = [&projView](i32 i){
    return vec4(projView[0][i], projView[1][i], projView[2][i], projView[3][i]);
  };

  m_planes = {
    row(3) + row(0),
    row(3) - row(0),
    row(3) + row(1),
    row(3) - row(1),
    row(2),
    row(3) - row(2)
  };
}

bool Camera::Frustum::intersects(vec3 mn, vec3 mx) const {
  for (const vec4& p : m_planes) {
    vec3 v(
      p.x > 0.0f ? mx.x : mn.x,
      p.y > 0.0f ? mx.y : mn.y,
      p.z > 0.0f ? mx.z : mn.z
    );
    if (glm::dot(vec3(p), v) + p.w < 0.0f)
      return false;
  }
  return true;
}


void Graveyard::retire(ValidType resource, u32 frameDelay) {
  m_retired.emplace_back(frameDelay, resource);
}

void Graveyard::update() {
  for (auto& [f, r] : m_retired) --f;
  std::erase_if(m_retired, [](auto r){ return r.first == 0; });
}