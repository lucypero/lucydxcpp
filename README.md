# Lucy's dx11 game

WIP

For now this is just to learn 3D rendering basics as I follow a [DirectX 11 book](https://www.amazon.com/Introduction-3D-Game-Programming-DirectX/dp/1936420228)

# How to build

- Visual Studio (including the Windows SDK and the latest MSVC compiler) and cmake need to be installed

- While installing Visual Studio, pick to install the Windows SDK and the latest MSVC compiler in the Visual Studio Installer

On a Developer Command Prompt for VS, `cd` to the repo folder and run...

```
mkdir build
build_shaders
cd build
cmake ..
```

It should generate a Visual Studio solution by default.