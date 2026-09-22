# Building for Windows

- [building](index.md)
- [docs home](../index.md)
- [readme](../../README.md)

The build uses Clang/GCC-style compiler flags, so MSVC's `cl.exe` won't work and you will need to install a separate compiler. The instructions below use Clang. Clang on Windows still uses the MSVC standard library and Windows SDK, so you also need the [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) with the "Desktop development with C++" workload installed.

Install the requirements:

```sh
winget install -e --id Kitware.CMake             # CMake
winget install -e --id Ninja-build.Ninja         # Ninja
winget install -e --id LLVM.LLVM                 # Clang/Clang++
winget install -e --id KhronosGroup.VulkanSDK    # Vulkan
```

Add the LLVM binaries to your path by pressing the `Win` key, searching `environment`, then click
`Edit the system environment variables`. Edit the `PATH` variable and add the LLVM binary directory (`C:\Program Files\LLVM\bin` by default).

Clone the repo
```sh
git clone https://github.com/hippgroot/cave-game
```

Pull the submodules
```sh
git submodule update --init --recursive
```

Configure a build. [Click here](index.md) for preset information
> If you didn't add LLVM to your path, use the exact paths to `clang.exe` and `clang++.exe` instead
```sh
cmake --preset win-<preset> -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Build the configuration. Make sure that `<preset>` is a preset that you have already configured
```sh
cmake --build --preset win-<preset>
```

Run the game
```sh
build\win-<preset>\cave-game.exe
```
