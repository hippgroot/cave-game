#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(binding = 0) uniform texture2D _Textures[];
layout(binding = 2) uniform sampler _Samplers[];

layout(buffer_reference, scalar) readonly buffer CameraBuffer {
  mat4  view;
  mat4  proj;
  mat4  projView;
  mat4  invView;
  mat4  invProj;
  vec3  position;
  vec3  forward;
  uvec2 viewport;
  float near;
  float far;
};

layout(buffer_reference) readonly buffer TextureLUT {
  uint heapIndices[];
};

layout(buffer_reference, scalar) readonly buffer GlobalsBuffer {
  CameraBuffer  cameraBuffer;
  TextureLUT    textureLUT;
  uint          nearestClampSampler;
  float         dt;
};

layout(push_constant) uniform PC {
layout(offset = 16)
  GlobalsBuffer globalsBuffer;
} _Constants;

layout(location = 0) in vec2 _UV;
layout(location = 1) in vec3 _Normal;
layout(location = 2) in flat uint _Block;

layout(location = 0) out vec4 _FragColor;

#define TEXLUT _Constants.globalsBuffer.textureLUT.heapIndices
#define SAMPLER _Constants.globalsBuffer.nearestClampSampler

void main() {
  const vec3 _lightDir = normalize(vec3(0.5, 1.0, 0.3));
  const vec3 _lightCol = vec3(1.0, 0.97, 0.9);
  const float _diffStr = 0.6;
  const vec3 _ambCol = vec3(0.6, 0.7, 1.0);
  const float _ambStr = 0.45;

  uint texIdx = TEXLUT[_Block];
  vec4 texCol = texture(
    sampler2D(_Textures[nonuniformEXT(texIdx)], _Samplers[SAMPLER]),
    _UV
  );

  vec3 albedo = texCol.rgb;
  vec3 ambient = _ambStr * _ambCol * albedo;
  vec3 diffuse = _diffStr * _lightCol * albedo * max(dot(_lightDir, _Normal), 0.0);

  _FragColor = vec4(ambient + diffuse, texCol.a);
}
