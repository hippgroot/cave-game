#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex {
  vec3 pos;
  vec2 uv;
  vec3 norm;
};

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
  Vertex vertices[];
};

layout(buffer_reference) readonly buffer BlocksBuffer {
  uint blocks[];
};

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

layout(push_constant) uniform PC {
  VertexBuffer  vertexBuffer;
  BlocksBuffer  blocksBuffer;
  CameraBuffer  cameraBuffer;
  ivec2         chunkCoord;
  int           chunkSize;
} _Constants;

layout(location = 0) out vec2 _UV;
layout(location = 1) out vec3 _Normal;
layout(location = 2) out flat uint _Block;

#define VERTICES _Constants.vertexBuffer.vertices
#define BLOCKS _Constants.blocksBuffer.blocks
#define CAMERA _Constants.cameraBuffer

void main() {
  Vertex v = VERTICES[gl_VertexIndex];

  vec3 chunkCoord = vec3(_Constants.chunkCoord.x, 0.0, _Constants.chunkCoord.y);
  vec3 chunkCenter = (chunkCoord - vec3(0.5, 0.0, 0.5)) * float(_Constants.chunkSize);
  vec3 pos = chunkCenter + v.pos;

  gl_Position = CAMERA.projView * vec4(pos, 1.0);
  _UV = v.uv;
  _Normal = v.norm;
  _Block = BLOCKS[gl_VertexIndex / 4];
}
