# Cave Game

A voxel game project aimed at the question "How would I design Minecraft if I were Notch?", recreating a Minecraft Alpha-style game with modern GPU rendering techniques. It is built with my Vulkan based rendering framework, [GRF](https://github.com/hippgroot/GRF).

## More to read

- [docs](docs/index.md)
- [building cave game](docs/building/index.md)
- [architecture](docs/architecture.md)
- [networking](docs/networking.md)
- [client UI](docs/ui.md)

## Quick start

Requires CMake 4.2+, Ninja, a C++26 compiler, and Vulkan. See [building](docs/building/index.md) for per-platform setup.

```sh
git clone https://github.com/hippgroot/cave-game
cd cave-game
git submodule update --init --recursive
cmake --workflow --preset <win|mac|lin>-<debug|relwdeb|release>
build/<preset>/src/client/cave-game
```

## Project Organization

| __folder__    | __target__  | __usage__                                                                   |
| ---           | ---         | ---                                                                         |
| `assets`      |             | shaders, textures, fonts, `blocks.yaml`                                     |
| `docs`        |             | design notes and build instructions                                         |
| `src/core`    | `core`      | `ThreadBus` and `Channel`, logging, config paths, atomic file writes, type aliases |
| `src/engine`  | `engine`    | `Renderer`, `Camera`, `Assets`, `Graveyard`, `MeshData`                     |
| `src/net`     | `net`       | message protocol, binary codec, `Link`, ENet `Server` and `Client`          |
| `src/game`    | `game`      | `Game`, the fixed-tick authoritative simulation                             |
| `src/client`  | `cave-game` | the executable: `main.cpp`, `Session`, settings, world and server lists, UI screens |
| `external`    |             | submodule dependencies (GRF, yaml-cpp, ENet, FastNoiseLite)                 |
