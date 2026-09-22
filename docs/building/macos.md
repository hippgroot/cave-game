# Building for MacOS

- [building](index.md)
- [docs home](../index.md)
- [readme](../../README.md)

Unless you want a different compiler, you can just use Apple Clang. The only real requirement is Vulkan, which you can install either from the [LunarG VulkanSDK](https://vulkan.lunarg.com/sdk/home) or through Homebrew. For Homebrew, install the following packages:

```sh
brew install cmake ninja molten-vk vulkan-headers vulkan-loader vulkan-tools vulkan-utility-libraries vulkan-validationlayers shaderc spirv-tools spirv-cross spirv-headers
```

For Homebrew, you need to export some environment variables as well to finish the setup

```sh
echo 'export BREW_PREFIX="$(brew --prefix)"' >> ~/.zprofile && \
echo 'export VK_LAYER_PREFIX="$(brew --prefix vulkan-validationlayers)"' >> ~/.zprofile && \
echo 'export VK_ICD_PREFIX="$(brew --prefix molten-vk)"' >> ~/.zprofile && \
echo 'export VULKAN_SDK="$BREW_PREFIX"' >> ~/.zprofile && \
echo 'export VK_DRIVER_FILES="$VK_ICD_PREFIX/etc/vulkan/icd.d/MoltenVK_icd.json"' >> ~/.zprofile && \
echo 'export VK_LAYER_PATH="$VK_LAYER_PREFIX/share/vulkan/explicit_layer.d"' >> ~/.zprofile
```

Once this is ready, clone the repo
```sh
git clone https://github.com/hippgroot/cave-game
```

Pull the submodules
```sh
git submodule update --init --recursive
```

Configure a build. [Click here](index.md) for preset information
> If using a different compiler, add the `CC` and `CXX` variables to your environment
```sh
cmake --preset mac-<preset>
```

Build the configuration. Make sure that `<preset>` is a preset that you have already configured
```sh
cmake --build --preset mac-<preset>
```

Run the game
```sh
build/mac-<preset>/cave-game
```
