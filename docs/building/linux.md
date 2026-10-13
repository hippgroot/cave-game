# Building for Linux

- [building](index.md)
- [docs home](../index.md)
- [readme](../../README.md)

Make sure you have CMake 4.2+, Ninja, a C++26-capable compiler, and Vulkan installed, either through your package manager or the [LunarG VulkanSDK](https://vulkan.lunarg.com/sdk/home). Most distros ship an older CMake, so you may need to get it from [Kitware's repository](https://apt.kitware.com/) or `pip install cmake`.

If you are not using the LunarG SDK, you also need the shaderc development package (`libshaderc-dev` on Debian/Ubuntu, `shaderc` on Arch). GLFW is built from source, so the X11/Wayland development headers are required as well (`xorg-dev libwayland-dev libxkbcommon-dev` on Debian/Ubuntu).

Once your environment is set up, clone the repo
```sh
git clone https://github.com/hippgroot/cave-game
```

Pull the submodules
```sh
git submodule update --init --recursive
```

Configure a build. [Click here](index.md) for preset information
```sh
cmake --preset lin-<preset>
```

Build the configuration. Make sure that `<preset>` is a preset that you have already configured
```sh
cmake --build --preset lin-<preset>
```

Run the game
```sh
build/lin-<preset>/src/client/cave-game
```
