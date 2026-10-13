# CLAUDE.md

Cave Game is a Minecraft Alpha-style voxel game in C++26 on Vulkan, built on the owner's own rendering framework GRF (`external/grf`). The `refactor` branch is a rebuild on a new architecture: menus, settings, world and server lists, the integrated server, and LAN/remote networking work; world generation, meshing, and in-game rendering are not ported yet.

Read `docs/architecture.md` before making structural changes. `docs/networking.md` and `docs/ui.md` cover those subsystems.

## Build and run

```sh
cmake --workflow --preset mac-debug
CAVE_LOG=debug build/mac-debug/src/client/cave-game
```

- Presets are `<win|mac|lin>-<debug|relwdeb|release>`. Configure with `cmake --preset <p>`, build with `cmake --build --preset <p>`.
- The executable is at `build/<preset>/src/client/cave-game`.
- `.clangd` points at `build/mac-release`, so build that preset (or edit `.clangd`) to keep `compile_commands.json` current for the language server.
- `CAVE_LOG` takes `level` or `category=level` pairs, e.g. `info,net=trace`. Categories: `core`, `net`, `game`, `client`, `render`.
- There is no test suite. Verify changes by building and, when behavior changes, running the game.

## Layout

| path          | target      | depends on                 |
| ---           | ---         | ---                        |
| `src/core`    | `core`      | GRF types, Threads         |
| `src/engine`  | `engine`    | core, GRF, yaml-cpp        |
| `src/net`     | `net`       | core, ENet (private)       |
| `src/game`    | `game`      | core, net                  |
| `src/client`  | `cave-game` | engine, game, yaml-cpp     |

- The include root is `src/`; include by module path (`#include "net/link.hpp"`), with GRF as `<grf/...>`.
- Every module's `CMakeLists.txt` lists its sources explicitly. Add new `.hpp`/`.cpp` files there.
- `game` must not include anything from `engine` or `client`; it is meant to be linkable into a future headless server.
- Assets are read from `ASSETS_DIR`, an absolute path to `assets/` baked in at configure time.

## Threading rules

- Cross-thread communication goes through `THREAD_BUS` channels or `net::Link`. Don't add shared mutable state between threads.
- `ThreadBus::send<T>` requires an explicit `T`. A message sent or received with the wrong type silently goes nowhere.
- ImGui is main-thread only. The renderer must draw the GUI with `GUI::render(cmd, snapshot)` using the `GUI::DrawData` from `FrameData`, never the overload that calls `ImGui::Render()`.
- Long-lived threads are `std::jthread` members stopped in the owner's destructor, and start their body with `logging::ThreadScope`.
- `Session` only sees a `ClientLink` and `Game` only sees `ServerLink`s. Keep local and remote play indistinguishable to both.

## Conventions

- Format with the repo's `.clang-format` (LLVM based, no column limit, aligned declarations and assignments).
- Don't write code comments. Put explanations in `docs/` or in the conversation.
- Types are `PascalCase`, functions and variables `camelCase`, members `m_name`, file-local constants `_name` in an anonymous namespace, namespaces lowercase.
- Use the aliases from `core/types.hpp` (`u32`, `f32`, `str`, `vec3`, ...).
- Classes explicitly delete or default all copy and move operations. Constructors that need several arguments take a nested `Info` struct with designated initializers.
- Log through `logging::<category>.info(...)` and friends, not `std::print`.
- Persisted files are YAML under `paths::config()` and are written with `files::writeAtomic`.
- When adding a protocol message, follow the checklist in `docs/networking.md` (append to the variant, bump `protocol::version`).

## Working with the owner

- The owner designs and writes game, simulation, and networking logic themselves. For those areas, default to reviewing, debugging, and advising rather than writing the code, unless asked.
- Engine/infrastructure work and client UI screens are fine to implement when asked.
- GRF is the owner's library. Changes there go in `external/grf` (a submodule with its own commits and docs in `external/grf/docs/`).
- When a change adds or alters a system, update the matching page in `docs/`.
