# Building

- [docs home](../index.md)
- [readme](../../README.md)

Every platform needs:

- CMake 4.2+
- Ninja
- A C++26-capable compiler (Clang 18+, GCC 14+, or Apple Clang)
- Vulkan

Everything else (GRF, GLFW, ImGui, yaml-cpp, ENet, FastNoiseLite) is a git submodule under `external/` and is built from source, so remember to pull submodules after cloning.

## Presets

There are three build presets

| __build preset__ | __description__                  |
| ---              | ---                              |
| debug            | debug build                      |
| relwdeb          | release build with debug symbols |
| release          | release build                    |

Combine the build preset with an operating system

| __OS__  | __tag__ |
| ---     | ---     |
| Windows | win     |
| macOS   | mac     |
| Linux   | lin     |

Configure with:

```sh
cmake --preset <OS tag>-<build preset>
```

Build with:

```sh
cmake --build --preset <OS tag>-<build preset>
```

To configure and build in one step, use the matching workflow preset instead:

```sh
cmake --workflow --preset <OS tag>-<build preset>
```

The executable is written to `build/<OS tag>-<build preset>/src/client/cave-game`.

## Per Platform Instructions

- [Windows](windows.md)
- [macOS](macos.md)
- [Linux](linux.md)

## Running

The asset directory is baked into the binary at configure time (`ASSETS_DIR` points at `assets/` in the source tree), so the game can be launched from any working directory but always reads assets from the checkout it was built from.

### Logging

Logs go to `stderr`, one line per message with uptime, thread name, category, and level. Color is used when `stderr` is a terminal and `NO_COLOR` is unset.

Levels are set with the `CAVE_LOG` environment variable, a comma separated list of either a bare level (applies to every category) or `category=level`:

```sh
CAVE_LOG=debug build/mac-debug/src/client/cave-game
CAVE_LOG=info,net=trace,game=debug build/mac-debug/src/client/cave-game
```

| __categories__                            | __levels__                                    |
| ---                                       | ---                                           |
| `core`, `net`, `game`, `client`, `render` | `trace`, `debug`, `info`, `warn`, `error`, `off` |

`net=trace` logs every protocol message sent and received.

### Saved files

Settings, the server list, and worlds are stored as YAML in a per-user config directory:

| __OS__  | __directory__                                                  |
| ---     | ---                                                            |
| Windows | `%APPDATA%\CaveGame`                                           |
| macOS   | `~/Library/Application Support/CaveGame`                       |
| Linux   | `$XDG_CONFIG_HOME/cave-game`, falling back to `~/.config/cave-game` |

| __file__                 | __contents__                         |
| ---                      | ---                                  |
| `settings.yaml`          | general, controls, and video settings |
| `servers.yaml`           | the multiplayer server list          |
| `saves/<world>/world.yaml` | world name, seed, last played time |

Delete a file to reset it to defaults.

## Usage with clangd

`CMAKE_EXPORT_COMPILE_COMMANDS` is on for every preset. Point clangd at the build you are using with a `.clangd` file in the root of the repo. Headers are never in `compile_commands.json`, so clangd guesses their flags from a nearby source file and can pick one from GRF, which is missing this project's include directories. The second block adds them back for every file under `src/` (the paths are relative to the build directory):

```yaml
CompileFlags:
  CompilationDatabase: build/<OS tag>-<build preset>
---
If:
  PathMatch: [src/.*]
CompileFlags:
  Add: [-I../../src, -I../../external/grf/include, -I../../external/enet/include, -I../../external/yaml-cpp/include]
```
