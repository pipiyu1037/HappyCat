# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Windows (Visual Studio)
```bash
# Configure
cmake -B build -G "Visual Studio 17 2022"
# Build
cmake --build build --config Release
# Run
./build/bin/Release/TriangleDemo.exe
```

### Linux
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release
# Build
cmake --build build
# Run
./build/bin/TriangleDemo
```

### Dependencies Setup
Dependencies are in `third_party/` and configured via scripts:
```bash
# Windows
cd scripts && ./setup_deps_windows.ps1
# Linux
cd scripts && ./setup_deps_linux.sh
```

## Architecture Overview

HappyCat is a Vulkan 1.3 rendering engine with 5 main modules:

| Module | Purpose |
|--------|---------|
| `Core` | Base types, logging, math (GLM), threading |
| `RHI/Vulkan` | Vulkan API wrappers (VKDevice, VKQueue, VKPipeline, etc.) |
| `Renderer/RenderGraph` | Compile-time static render graph with automatic barriers |
| `Platform` | Window abstraction (GLFW), input management |
| `Engine` | Application framework, frame context, main loop |

### Module Dependencies
```
Engine → Platform + Renderer + RHI
Renderer → RHI + Core
RHI/Vulkan → Core
Platform → Core
```

## Key Patterns

### Factory Pattern
Most Vulkan wrappers use static `Create()` factory methods returning `std::unique_ptr`:
```cpp
static std::unique_ptr<VKDevice> Create(const CreateInfo& info);
```

### Non-copyable Types
Vulkan resource classes are non-copyable (delete copy ctor/assignment).

### Smart Pointer Aliases
```cpp
template<typename T> using Scope = std::unique_ptr<T>;
template<typename T> using Ref = std::shared_ptr<T>;
```

### Type Aliases
```cpp
using i8/i16/i32/i64 = int*_t;
using u8/u16/u32/u64 = uint*_t;
using f32 = float; using f64 = double;
```

## Naming Conventions

- **Namespace**: `happycat::`
- **Classes**: PascalCase (e.g., `VKDevice`, `RenderPass`)
- **Vulkan wrappers**: `VK` prefix (e.g., `VKCommandBuffer`)
- **Member variables**: `m_` prefix (e.g., `m_Device`)
- **Getters**: `GetHandle()` for Vulkan handles, `Get*()` for objects

## RenderPass Interface

To create a custom render pass, inherit from `RenderPass`:
```cpp
class MyPass : public RenderPass {
public:
    const char* GetName() const override { return "MyPass"; }
    void DeclareResources(ResourceBuilder& builder) override { }
    void DeclareDependencies(DependencyBuilder& builder) override { }
    void Execute(RenderPassContext& ctx, VKCommandBuffer& cmd) override;
};
```

## File Organization

```
src/
├── Core/
│   ├── Utils/Types.h      # Base types, smart pointer aliases
│   ├── Math/MathTypes.h   # GLM type aliases
│   └── Threading/         # ThreadPool
├── RHI/
│   ├── RHITypes.h         # Format enums, usage flags
│   └── Vulkan/            # VK* classes (one .h/.cpp per type)
├── Renderer/RenderGraph/
│   ├── RenderPass.h       # Base class for passes
│   ├── RenderResource.h   # TextureHandle, BufferHandle
│   └── RenderGraphBuilder.h
├── Platform/
│   └── Window/HCWindow.h
└── Engine/
    └── Application.h      # Base application class
```

## Vulkan Initialization Order

1. `VKInstance` → 2. `VKPhysicalDevice` → 3. `VKDevice` → 4. `VKQueue` → 5. `VKSwapChain`

## Current Status

Phase 1: Minimal runnable framework - **COMPLETED**
Phase 2: Core rendering features - **IN PROGRESS**

### Completed Modules
- **Core**: Base types, logging (spdlog), math (GLM), threading (ThreadPool)
- **RHI/Vulkan**: Full Vulkan 1.3 wrappers (Instance, Device, Queue, SwapChain, Pipeline, DescriptorSet, Sampler, etc.)
- **Renderer/RenderGraph**: Static render graph with resource/dependency builders
- **Platform**: GLFW window abstraction, input management
- **Engine**: Application framework, frame context, main loop

### Working Samples
- `samples/01_triangle/` - Triangle Demo (colorful triangle, runtime shader compilation)
- `samples/02_rendergraph/` - RenderGraph Demo (blue triangle, demonstrates render graph usage)
- `samples/03_textured_quad/` - TexturedQuad Demo (MVP uniform buffer, descriptor sets, rotating quad)
- `samples/04_pbr_material/` - PBR Material Demo (PBR sphere, material system, TBN matrix for normal mapping)

### Key Features
- ✅ Runtime GLSL → SPIR-V compilation (shaderc)
- ✅ Smart fallback: loads pre-compiled .spv first, compiles GLSL source if not found
- ✅ Resource tracking system for leak detection
- ✅ Window resize handling with SwapChain recreation
- ✅ Vulkan Validation Layers clean
- ✅ DescriptorSet system (Uniform Buffer, CombinedImageSampler support)

### Fixed Issues
- FrameContext initialization bug
- Window resize crash
- Resource leak detection
- Vulkan SDK static library MSVC compatibility (using shaderc DLL instead)
- VKBuffer memory allocation sType not set
- DescriptorType enum duplicate values

### Next Steps (Phase 2)
- ✅ DescriptorSet system - **COMPLETED**
- ✅ PBR material system - **COMPLETED**
- Mesh loading (OBJ/GLTF)
- Camera system
- Lighting system
