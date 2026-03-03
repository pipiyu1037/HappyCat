# HappyCat

A modern Vulkan-based rendering engine with Render Graph architecture.

## Features

- **Vulkan 1.3** - Latest Vulkan API with dynamic rendering support
- **Render Graph** - Compile-time static render graph with automatic resource barrier management
- **PBR Rendering** - Physically-based rendering with metallic-roughness workflow
- **Cross-Platform** - Windows and Linux support

## Requirements

- **CMake** 3.20+
- **C++17** compatible compiler
- **Vulkan SDK** 1.3+

### Windows
- Visual Studio 2022 or later
- Windows 10/11 SDK

### Linux
- GCC 9+ or Clang 10+
- X11 or Wayland development packages

## Building

### 1. Install Dependencies

**Windows:**
```powershell
cd scripts
.\setup_deps_windows.ps1
```

**Linux:**
```bash
cd scripts
chmod +x setup_deps_linux.sh
./setup_deps_linux.sh
```

### 2. Build Project

**Windows:**
```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

**Linux:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 3. Run Samples

```bash
./build/bin/Release/TriangleDemo
```

## Project Structure

```
HappyCat/
├── src/
│   ├── Core/           # Core utilities (types, logger, math)
│   ├── RHI/            # Render Hardware Interface
│   │   └── Vulkan/     # Vulkan implementation
│   ├── Renderer/       # Rendering core
│   │   └── RenderGraph/
│   ├── Platform/       # Platform abstraction (window, input)
│   └── Engine/         # Engine entry point
├── shaders/            # GLSL shaders
├── samples/            # Example applications
├── third_party/        # Dependencies
└── scripts/            # Build scripts
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| GLFW | 3.4 | Window & Input |
| glm | 1.0.1 | Math library |
| spdlog | 1.14.1 | Logging |
| Dear ImGui | docking | Debug UI |
| glslang | 14.3.0 | Shader compilation |
| SPIRV-Cross | 1.3.290.0 | Shader reflection |
| stb | latest | Image loading |
| tinyobjloader | 2.0.0 | OBJ loading |
| tinygltf | 2.9.2 | GLTF loading |

## License

MIT License

## References

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Render Graph Architecture](https://blog.traverseresearch.nl/render-graphs-101-7128afc03653)
- [PBR Theory](https://learnopengl.com/PBR/Theory)
