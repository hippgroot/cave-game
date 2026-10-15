#include "game/world/gen/stages/finalize.hpp"

void FinalizeStage::generate(Region& region) const {
  ProtoChunk& proto = region.self();

  for (i32 z = 0; z < Chunk::width; ++z) {
    for (i32 x = 0; x < Chunk::width; ++x) {
      i16 height = -1;
      for (i32 y = Chunk::height - 1; y >= 0; --y) {
        const Block block = proto.chunk.get({x, y, z});
        if (block != Block::Air && block != Block::Water) {
          height = static_cast<i16>(y);
          break;
        }
      }
      proto.heightmap[ProtoChunk::column(x, z)] = height;
    }
  }
}
