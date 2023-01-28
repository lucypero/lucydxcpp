# Lucy's dx11 game

For now this is just to learn 3D rendering basics as I follow a [DirectX 11 book](https://www.amazon.com/Introduction-3D-Game-Programming-DirectX/dp/1936420228)

# How to build

- Visual Studio and cmake need to be installed

- While installing Visual Studio, pick to install the Windows SDK and the latest MSVC compiler in the Visual Studio Installer

From a Developer Command Prompt for VS, `cd` to the repo folder and run...

```
mkdir build
build_shaders.bat
cd build
cmake ..
```

It should generate a Visual Studio solution by default.

If `build_shaders.bat` fails, it might be because your `fxc.exe` is in another directory. You should have it if you have the Windows SDK installed using the Visual Studio Installer.