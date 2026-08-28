#version 460
#extension GL_EXT_buffer_reference : require

layout(buffer_reference) readonly buffer VertexBuffer {
  vec4 vertices[];
};

layout(push_constant) uniform PC {
  VertexBuffer _VertexBuffer;
  mat4 _ProjView;
};

void main() {
  gl_Position = _ProjView * _VertexBuffer.vertices[gl_VertexIndex];
}
