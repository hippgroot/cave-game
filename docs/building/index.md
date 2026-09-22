# Building

- [docs home](../index.md)
- [readme](../../README.md)

Every platform needs:

- CMake 4.2+
- Ninja
- A C++26-capable compiler (Clang 18+, GCC 14+, or Apple Clang)
- Vulkan

There are three different build presets

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

The final CMake command follows this structure:

```sh
cmake --preset <OS tag>-<build preset>
```

To configure and build in one step, use the matching workflow preset instead:

```sh
cmake --workflow --preset <OS tag>-<build preset>
```

## Per Platform Instructions

- [Windows](windows.md)
- [macOS](macos.md)
- [Linux](linux.md)


## Usage with clangd

If using clangd, you will need to set up a `.clangd` file in the root of the repo to point to the correct build compile commands. Headers are never in `compile_commands.json`, so clangd guesses their flags from a nearby source file and can pick one from GRF, which is missing this project's include directories. The second block adds them back for every file under `client/`, `game/`, and `engine/` (the paths are relative to the build directory):

```yaml
CompileFlags:
  CompilationDatabase: build/<OS tag>-<build preset>
---
If:
  PathMatch: [client/.*, game/.*, engine/.*]
CompileFlags:
  Add: [-I../.., -I../../external/FastNoiseLite/Cpp, -I../../external/grf/include]
```
