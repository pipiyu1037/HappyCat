# HappyCat 渲染引擎 - 技术规格说明书

> 版本: 1.0
> 日期: 2026-03-02
> 状态: 需求定义

---

## 1. 项目概述

### 1.1 项目名称
**HappyCat** - 基于 Vulkan 的现代渲染引擎

### 1.2 项目定位
HappyCat 是一个完整的跨平台渲染引擎，核心特性包括：
- 基于 Vulkan 1.3 的底层渲染后端
- 编译时静态 Render Graph 架构
- 可扩展的 Render Pass 管线系统
- PBR 物理渲染支持

### 1.3 技术栈
| 项目 | 选择 |
|------|------|
| 编程语言 | C++17 |
| 图形 API | Vulkan 1.3 |
| 构建系统 | CMake |
| 窗口系统 | GLFW |
| UI 系统 | Dear ImGui |
| Shader 语言 | GLSL (运行时编译) |
| 目标平台 | Windows, Linux |

---

## 2. 架构设计

### 2.1 模块划分

```
HappyCat/
├── Core/                    # 核心基础设施
│   ├── Memory/              # 内存管理、自定义分配器
│   ├── Container/           # 容器封装
│   ├── Threading/           # 线程池、任务调度
│   ├── Math/                # 数学库 (Vec, Mat, Quaternion)
│   └── Utils/               # 工具函数、日志系统
│
├── RHI/                     # Render Hardware Interface 抽象层
│   ├── Vulkan/              # Vulkan 具体实现
│   │   ├── Device/          # 逻辑设备、物理设备管理
│   │   ├── Memory/          # 显存管理、Allocator
│   │   ├── Pipeline/        # 管线状态对象
│   │   ├── Command/         # 命令缓冲区管理
│   │   ├── Synchronization/ # 同步原语 (Fence, Semaphore, Barrier)
│   │   └── Descriptor/      # Descriptor Set 管理
│   └── RHITypes.h           # RHI 类型定义
│
├── Renderer/                # 渲染核心
│   ├── RenderGraph/         # Render Graph 实现
│   │   ├── GraphBuilder/    # 图构建器
│   │   ├── ResourceTracker/ # 资源追踪与屏障自动管理
│   │   ├── Scheduler/       # 多队列调度器
│   │   └── Compiler/        # 图编译与优化
│   ├── RenderPass/          # Render Pass 基类与内置 Pass
│   └── FrameGraph/          # 帧图管理
│
├── Resource/                # 资源管理
│   ├── Texture/             # 纹理加载与管理
│   ├── Mesh/                # 网格加载与管理
│   ├── Material/            # 材质系统
│   ├── Shader/              # Shader 编译与反射
│   └── Cache/               # 资源缓存与热重载
│
├── Scene/                   # 场景管理
│   ├── Entity/              # 实体/组件
│   ├── Camera/              # 相机系统
│   ├── Light/               # 光源系统
│   └── Transform/           # 变换层级
│
├── PostProcess/             # 后处理栈
│   ├── Bloom/               # 泛光
│   ├── ToneMapping/         # HDR 色调映射
│   └── AntiAliasing/        # 抗锯齿 (FXAA/TAA)
│
├── Platform/                # 平台抽象层
│   ├── Window/              # 窗口管理 (GLFW)
│   └── Input/               # 输入系统
│
└── Engine/                  # 引擎入口
    ├── Application/         # 应用程序框架
    └── MainLoop/            # 主循环
```

### 2.2 依赖关系

```
┌─────────────────────────────────────────────────────┐
│                      Engine                         │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌─────────┐  ┌──────────┐  ┌─────────────────┐   │
│  │ Platform│  │PostProcess│  │     Scene       │   │
│  └────┬────┘  └────┬─────┘  └────────┬────────┘   │
│       │            │                  │            │
│       └────────────┼──────────────────┘            │
│                    │                               │
│              ┌─────┴─────┐                         │
│              │  Renderer │                         │
│              └─────┬─────┘                         │
│                    │                               │
│       ┌────────────┼────────────┐                  │
│       │            │            │                  │
│  ┌────┴───┐  ┌─────┴─────┐  ┌───┴────┐            │
│  │  RHI   │  │ Resource  │  │  Core  │            │
│  └────────┘  └───────────┘  └────────┘            │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## 3. Render Graph 设计

### 3.1 核心概念

#### 3.1.1 资源声明
```cpp
// 资源在 Graph 中声明，由 Graph 管理生命周期
class RenderResource {
public:
    enum class Type {
        Texture,
        Buffer,
        Reference  // 外部资源引用
    };

    Type GetType() const;
    ResourceHandle GetHandle() const;
};
```

#### 3.1.2 Render Pass 接口
```cpp
class RenderPass {
public:
    virtual ~RenderPass() = default;

    // Pass 元信息
    virtual const char* GetName() const = 0;
    virtual uint32_t GetPriority() const { return 0; }

    // 声明资源依赖
    virtual void DeclareResources(ResourceBuilder& builder) = 0;

    // 声明 Pass 间的执行依赖
    virtual void DeclareDependencies(DependencyBuilder& builder) = 0;

    // 执行条件（可选）
    virtual bool ShouldExecute(const FrameContext& ctx) const { return true; }

    // 录制命令
    virtual void Execute(RenderPassContext& ctx, CommandBuffer& cmd) = 0;

    // 多线程录制支持
    virtual void ExecuteParallel(RenderPassContext& ctx,
                                 CommandBuffer& cmd,
                                 uint32_t workerIndex,
                                 uint32_t workerCount) {}

protected:
    // 多 Attachment 支持
    std::vector<AttachmentInfo> m_ColorAttachments;
    AttachmentInfo m_DepthAttachment;
    bool m_HasDepth = false;
};
```

#### 3.1.3 Render Graph 构建器
```cpp
class RenderGraphBuilder {
public:
    // 添加 Pass
    template<typename PassType, typename... Args>
    PassType& AddPass(Args&&... args);

    // 创建临时资源
    TextureHandle CreateTexture(const TextureDesc& desc);
    BufferHandle CreateBuffer(const BufferDesc& desc);

    // 导入外部资源
    TextureHandle ImportTexture(Texture* externalTexture);

    // 设置 Backbuffer
    void SetBackbuffer(TextureHandle backbuffer);

    // 编译图
    void Compile();

    // 执行图
    void Execute(FrameContext& ctx);

private:
    std::vector<std::unique_ptr<RenderPass>> m_Passes;
    ResourceAllocator m_ResourceAllocator;
    BarrierScheduler m_BarrierScheduler;
    QueueScheduler m_QueueScheduler;
};
```

### 3.2 自动资源屏障管理

```cpp
// Graph 编译时自动分析资源状态转换
class BarrierScheduler {
public:
    // 分析整个图的状态转换
    void AnalyzeResourceStates(const std::vector<RenderPass*>& passes);

    // 生成最优的屏障插入点
    void GenerateBarriers();

    // 支持自动 layout transition
    void InsertLayoutTransitions(CommandBuffer& cmd);

    // 支持自动 pipeline barrier
    void InsertPipelineBarriers(CommandBuffer& cmd);
};
```

### 3.3 资源别名 (Memory Aliasing)

```cpp
class ResourceAliasOptimizer {
public:
    // 分析资源生命周期，找出可以别名的情况
    void AnalyzeLifetimes(const std::vector<RenderResource>& resources);

    // 生成内存别名映射
    std::vector<AliasRange> GenerateAliasMap();

    // 在资源不重叠的生命周期内复用同一块显存
    size_t CalculateAliasedMemorySize();
};
```

### 3.4 多队列调度

```cpp
enum class QueueType {
    Graphics,
    Compute,
    Transfer
};

class QueueScheduler {
public:
    // 将 Pass 分配到不同队列
    void SchedulePasses(const std::vector<RenderPass*>& passes);

    // 生成跨队列同步
    void GenerateCrossQueueSync();

    // 获取队列执行计划
    const QueueExecutionPlan& GetPlan() const;
};
```

### 3.5 动态分辨率支持

```cpp
class DynamicResolutionController {
public:
    void SetScaleRange(float minScale, float maxScale);
    void SetTargetFrameTime(float targetMs);

    // 根据帧时间自动调整分辨率
    void Update(float frameTimeMs);

    float GetCurrentScale() const;
    uint32_t GetScaledWidth() const;
    uint32_t GetScaledHeight() const;
};
```

### 3.6 GPU 时间戳查询

```cpp
class TimestampQueryPool {
public:
    void Initialize(Device* device, uint32_t queryCount);

    // 在 Pass 开始/结束时写入时间戳
    void WriteTimestamp(CommandBuffer& cmd, uint32_t queryIndex,
                        PipelineStage stage);

    // 获取 Pass 执行时间
    float GetPassDuration(const RenderPass* pass) const;

    // 获取完整帧分析数据
    const FrameProfileData& GetProfileData() const;
};
```

### 3.7 使用示例

```cpp
class GBufferPass : public RenderPass {
public:
    const char* GetName() const override { return "GBuffer"; }

    void DeclareResources(ResourceBuilder& builder) override {
        // 创建 G-Buffer 附件
        m_AlbedoRT = builder.CreateTexture({
            .name = "Albedo",
            .format = Format::RGBA8_UNORM,
            .usage = TextureUsage::ColorAttachment | TextureUsage::ShaderRead
        });

        m_NormalRT = builder.CreateTexture({
            .name = "Normal",
            .format = Format::RGBA16_FLOAT,
            .usage = TextureUsage::ColorAttachment | TextureUsage::ShaderRead
        });

        m_DepthRT = builder.CreateTexture({
            .name = "Depth",
            .format = Format::D24_UNORM_S8_UINT,
            .usage = TextureUsage::DepthStencil | TextureUsage::ShaderRead
        });

        // 设置多附件
        m_ColorAttachments = { {m_AlbedoRT}, {m_NormalRT} };
        m_DepthAttachment = { m_DepthRT };
        m_HasDepth = true;
    }

    void DeclareDependencies(DependencyBuilder& builder) override {
        // 声明对场景数据的依赖
        builder.ReadResource(m_SceneDataBuffer);
    }

    void Execute(RenderPassContext& ctx, CommandBuffer& cmd) override {
        // 录制渲染命令...
    }

private:
    TextureHandle m_AlbedoRT, m_NormalRT, m_DepthRT;
    BufferHandle m_SceneDataBuffer;
};

// 构建渲染图
RenderGraphBuilder graph;

auto& gbufferPass = graph.AddPass<GBufferPass>();
auto& lightingPass = graph.AddPass<DeferredLightingPass>();
auto& postProcessPass = graph.AddPass<PostProcessPass>();

graph.Compile();
graph.Execute(frameContext);
```

---

## 4. 资源管理系统

### 4.1 资源生命周期管理

```cpp
template<typename ResourceType>
class ResourcePtr {
public:
    ResourcePtr();
    ResourcePtr(ResourceType* resource, ResourceManager* manager);

    // 引用计数
    ResourcePtr(const ResourcePtr& other);
    ResourcePtr& operator=(const ResourcePtr& other);

    // 自动释放
    ~ResourcePtr();

    ResourceType* Get() const;
    ResourceType* operator->() const;

private:
    ResourceType* m_Resource = nullptr;
    ResourceManager* m_Manager = nullptr;
};
```

### 4.2 纹理加载

```cpp
class TextureLoader {
public:
    // 支持格式
    enum class Format {
        PNG,
        JPEG,
        BMP,
        TGA,
        HDR,
        KTX  // 压缩纹理
    };

    ResourcePtr<Texture> Load(const std::string& path);
    ResourcePtr<Texture> LoadFromMemory(const void* data, size_t size);

    // 支持 mipmap 自动生成
    void GenerateMipmaps(Texture* texture);

private:
    std::unordered_map<std::string, ResourcePtr<Texture>> m_Cache;
};
```

### 4.3 模型加载

```cpp
class MeshLoader {
public:
    // 支持格式
    ResourcePtr<Mesh> LoadOBJ(const std::string& path);
    ResourcePtr<Mesh> LoadGLTF(const std::string& path);

    // 支持网格优化
    void OptimizeMesh(Mesh* mesh);

    // 支持 LOD 生成
    void GenerateLODs(Mesh* mesh, uint32_t lodCount);
};

class ModelLoader {
public:
    ResourcePtr<Model> LoadGLTF(const std::string& path);

    // 加载完整场景（包含材质、层级）
    ResourcePtr<Scene> LoadScene(const std::string& path);
};
```

### 4.4 资源热重载

```cpp
class HotReloadManager {
public:
    // 监视文件变化
    void WatchFile(const std::string& path, std::function<void()> callback);

    // 更新检测
    void Update();

    // 强制重载
    void ReloadAll();
    void ReloadResource(const std::string& path);

private:
    struct WatchEntry {
        std::string path;
        std::filesystem::file_time_type lastWrite;
        std::function<void()> callback;
    };
    std::vector<WatchEntry> m_WatchList;
};
```

---

## 5. Shader 系统

### 5.1 运行时编译

```cpp
class ShaderCompiler {
public:
    struct CompileOptions {
        std::vector<std::string> defines;
        std::string entryPoint = "main";
        ShaderStage stage;
        bool optimize = true;
    };

    // GLSL -> SPIR-V
    SpirvBinary Compile(const std::string& source,
                        const CompileOptions& options);

    // 从文件编译
    SpirvBinary CompileFromFile(const std::string& path,
                                const CompileOptions& options);

    // 错误处理
    const std::string& GetLastError() const;
};
```

### 5.2 Shader 反射

```cpp
struct ShaderReflection {
    struct DescriptorBinding {
        std::string name;
        uint32_t set;
        uint32_t binding;
        DescriptorType type;
        uint32_t count;
    };

    struct PushConstantRange {
        std::string name;
        uint32_t offset;
        uint32_t size;
    };

    struct VertexInput {
        std::string name;
        uint32_t location;
        Format format;
        uint32_t offset;
    };

    std::vector<DescriptorBinding> descriptorBindings;
    std::vector<PushConstantRange> pushConstants;
    std::vector<VertexInput> vertexInputs;
};

class ShaderReflector {
public:
    // 从 SPIR-V 提取反射信息
    ShaderReflection Reflect(const SpirvBinary& spirv);

    // 自动生成 Descriptor Set Layout
    DescriptorSetLayout CreateDescriptorSetLayout(Device* device,
                                                   const ShaderReflection& reflection);
};
```

### 5.3 Shader 模块

```cpp
class ShaderModule {
public:
    struct CreateInfo {
        std::string vertexPath;
        std::string fragmentPath;
        std::string geometryPath;
        std::string computePath;

        std::vector<std::string> defines;
    };

    static ShaderModule* Create(Device* device, const CreateInfo& info);

    PipelineLayout* GetPipelineLayout() const;
    const ShaderReflection& GetReflection() const;

private:
    VkShaderModule m_ShaderModules[5];
    PipelineLayout* m_PipelineLayout;
    ShaderReflection m_Reflection;
};
```

---

## 6. 同步与多线程

### 6.1 帧同步

```cpp
class FrameSyncManager {
public:
    void Initialize(Device* device, uint32_t framesInFlight);

    // 等待当前帧完成
    void WaitForFrame(uint64_t frameIndex);

    // 获取当前帧可用的资源
    uint32_t GetCurrentFrameIndex() const;

    // 帧资源
    struct FrameResources {
        VkFence renderFence;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        CommandPool commandPool;
        std::vector<CommandBuffer> commandBuffers;
    };

    FrameResources& GetCurrentFrameResources();

private:
    uint32_t m_FramesInFlight;
    std::vector<FrameResources> m_FrameResources;
};
```

### 6.2 多线程命令录制

```cpp
class ParallelCommandRecorder {
public:
    struct RecordingTask {
        RenderPass* pass;
        uint32_t workerIndex;
    };

    void Initialize(uint32_t threadCount);

    // 并行录制多个 Pass
    void RecordPasses(const std::vector<RecordingTask>& tasks);

    // 合并命令缓冲区
    void MergeCommandBuffers(CommandBuffer& primary,
                             const std::vector<CommandBuffer>& secondary);

private:
    ThreadPool m_ThreadPool;
    std::vector<CommandPool> m_ThreadCommandPools;
};
```

### 6.3 异步资源上传

```cpp
class StagingBufferManager {
public:
    void Initialize(Device* device, size_t stagingBufferSize);

    // 上传缓冲区数据
    BufferCopyRegion UploadBuffer(const void* data, size_t size,
                                  Buffer* dstBuffer, size_t dstOffset);

    // 上传纹理数据
    void UploadTexture(const void* data, const TextureRegion& region,
                       Texture* dstTexture);

    // 提交所有上传任务
    void FlushUploads();

    // 等待上传完成
    void WaitForUploads();

private:
    Buffer* m_StagingBuffer;
    size_t m_CurrentOffset;
    CommandBuffer* m_UploadCmdBuffer;
    VkFence m_UploadFence;
};
```

---

## 7. 内置 Render Pass

### 7.1 Shadow Pass

```cpp
class ShadowPass : public RenderPass {
public:
    // 支持级联阴影
    struct CascadeInfo {
        float splitDepth;
        Mat4 viewProjMatrix;
    };

    void SetCascadeCount(uint32_t count);
    void SetCascadeSplits(const std::vector<float>& splits);

    const std::vector<CascadeInfo>& GetCascadeData() const;

private:
    std::vector<CascadeInfo> m_Cascades;
    TextureHandle m_ShadowMapArray;  // Layered shadow maps
};
```

### 7.2 G-Buffer Pass

```cpp
class GBufferPass : public RenderPass {
public:
    // G-Buffer Layout:
    // RT0: Albedo (RGB) + Metallic (A)
    // RT1: Normal (RG) + Roughness (B) + AO (A)
    // RT2: Emissive (RGB) + Reserved (A)
    // Depth: Depth + Stencil

    struct GBuffer {
        TextureHandle albedoMetallic;
        TextureHandle normalRoughnessAO;
        TextureHandle emissive;
        TextureHandle depth;
    };

    const GBuffer& GetGBuffer() const;
};
```

### 7.3 Deferred Lighting Pass

```cpp
class DeferredLightingPass : public RenderPass {
public:
    void SetGBuffer(const GBufferPass::GBuffer& gbuffer);
    void SetShadowMap(const ShadowPass& shadowPass);

    // 光源数据
    void SetDirectionalLights(const std::vector<DirectionalLight>& lights);
    void SetPointLights(const std::vector<PointLight>& lights);
    void SetSpotLights(const std::vector<SpotLight>& lights);
    void SetIBL(const IBLData& ibl);

private:
    // 光源数据存储在 SSBO
    BufferHandle m_LightDataBuffer;
};
```

### 7.4 Forward Pass

```cpp
class ForwardPass : public RenderPass {
public:
    // 用于透明物体和不支持 Deferred 的材质

    void SetScene(Scene* scene);
    void SetCamera(Camera* camera);

    // 排序模式
    enum class SortMode {
        None,
        FrontToBack,
        BackToFront
    };

    void SetSortMode(SortMode mode);
};
```

### 7.5 Post-Processing Passes

```cpp
// Bloom Pass
class BloomPass : public RenderPass {
public:
    void SetThreshold(float threshold);
    void SetIntensity(float intensity);
    void SetRadius(float radius);

private:
    // 下采样 + 上采样实现
    std::vector<TextureHandle> m_DownsampleChain;
    std::vector<TextureHandle> m_UpsampleChain;
};

// Tone Mapping Pass
class ToneMappingPass : public RenderPass {
public:
    enum class Algorithm {
        Reinhard,
        ACES,
        Uncharted2,
        Filmic
    };

    void SetAlgorithm(Algorithm algo);
    void SetExposure(float exposure);
};

// Anti-Aliasing Pass
class AntiAliasingPass : public RenderPass {
public:
    enum class Method {
        None,
        FXAA,
        TAA
    };

    void SetMethod(Method method);

    // TAA 需要历史缓冲
    void SetHistoryBuffer(TextureHandle history);
};
```

---

## 8. PBR 材质系统

### 8.1 材质定义

```cpp
class PBRMaterial {
public:
    // PBR 纹理
    TextureHandle albedoMap;
    TextureHandle normalMap;
    TextureHandle metallicRoughnessMap;
    TextureHandle occlusionMap;
    TextureHandle emissiveMap;

    // 常量值（当纹理不存在时使用）
    Vec4 albedoColor = Vec4(1.0f);
    float metallic = 0.5f;
    float roughness = 0.5f;
    Vec3 emissiveColor = Vec3(0.0f);
    float occlusionStrength = 1.0f;

    // 渲染状态
    enum class BlendMode {
        Opaque,
        AlphaTest,
        AlphaBlend
    };
    BlendMode blendMode = BlendMode::Opaque;
    float alphaCutoff = 0.5f;

    // 双面渲染
    bool doubleSided = false;
};
```

### 8.2 光照系统

```cpp
// 方向光
struct DirectionalLight {
    Vec3 direction;
    Vec3 color;
    float intensity;
    bool castShadow;
};

// 点光源
struct PointLight {
    Vec3 position;
    Vec3 color;
    float intensity;
    float radius;
    bool castShadow;
};

// 聚光灯
struct SpotLight {
    Vec3 position;
    Vec3 direction;
    Vec3 color;
    float intensity;
    float innerConeAngle;
    float outerConeAngle;
    float radius;
    bool castShadow;
};

// IBL (Image-Based Lighting)
struct IBLData {
    TextureHandle diffuseEnvMap;   // 辐照度图
    TextureHandle specularEnvMap;  // 预过滤环境图
    TextureHandle brdfLUT;         // BRDF 查找表
    float mipLevels;
};
```

### 8.3 Shader 实现 (GLSL)

```glsl
// PBR 核心计算
vec3 evaluatePBR(
    vec3 N,           // 法线
    vec3 V,           // 视线方向
    vec3 L,           // 光线方向
    vec3 lightColor,  // 光源颜色
    float metallic,   // 金属度
    float roughness   // 粗糙度
) {
    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.001);
    float NdotL = max(dot(N, L), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Cook-Torrance BRDF
    float D = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(VdotH, F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    return (kD * albedo / PI + specular) * lightColor * NdotL;
}
```

---

## 9. 窗口与输入系统

### 9.1 窗口管理

```cpp
class Window {
public:
    struct CreateInfo {
        std::string title;
        uint32_t width;
        uint32_t height;
        bool fullscreen;
        bool resizable;
    };

    static Window* Create(const CreateInfo& info);

    void PollEvents();
    bool ShouldClose() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    // Vulkan 表面
    VkSurfaceKHR GetSurface() const;

    // 回调
    using ResizeCallback = std::function<void(uint32_t, uint32_t)>;
    void SetResizeCallback(ResizeCallback callback);

private:
    GLFWwindow* m_Window;
    VkSurfaceKHR m_Surface;
};
```

### 9.2 输入系统

```cpp
class InputManager {
public:
    void Initialize(Window* window);

    // 键盘
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyPressed(KeyCode key) const;   // 此帧按下
    bool IsKeyReleased(KeyCode key) const;  // 此帧释放

    // 鼠标
    bool IsMouseButtonDown(MouseButton button) const;
    Vec2 GetMousePosition() const;
    Vec2 GetMouseDelta() const;
    float GetMouseScroll() const;

    // 鼠标模式
    enum class CursorMode {
        Normal,
        Hidden,
        Disabled  // 锁定到窗口中心
    };
    void SetCursorMode(CursorMode mode);

    // 每帧更新
    void Update();

private:
    Window* m_Window;

    std::array<bool, 512> m_CurrentKeys;
    std::array<bool, 512> m_PreviousKeys;

    Vec2 m_MousePosition;
    Vec2 m_MouseDelta;
    float m_ScrollDelta;
};
```

---

## 10. ImGui 集成

### 10.1 集成实现

```cpp
class ImGuiRenderer {
public:
    void Initialize(Device* device, Window* window);
    void Shutdown();

    // 每帧调用
    void NewFrame();
    void Render(CommandBuffer& cmd);

    // 处理输入事件
    void OnMouseMove(float x, float y);
    void OnMouseButton(int button, bool pressed);
    void OnKey(int key, bool pressed);
    void OnChar(unsigned int c);
    void OnScroll(float delta);

private:
    void CreateFontsTexture();
    void SetupRenderState(CommandBuffer& cmd);

    TextureHandle m_FontTexture;
    BufferHandle m_VertexBuffer;
    BufferHandle m_IndexBuffer;
    Pipeline* m_Pipeline;
    DescriptorSet* m_DescriptorSet;
};
```

### 10.2 调试面板

```cpp
// 引擎内置调试 UI
class DebugPanel {
public:
    void Render();

private:
    void RenderPerformanceStats();
    void RenderRenderGraphViewer();
    void RenderResourceBrowser();
    void RenderMaterialEditor();

    // 性能统计
    struct Stats {
        float fps;
        float frameTime;
        float gpuTime;
        uint32_t drawCalls;
        uint32_t triangleCount;
        size_t memoryUsed;
    };
};
```

---

## 11. 开发阶段规划

### Phase 1: 基础框架 + Render Graph 核心
**目标**: 搭建最小可运行框架

- [ ] CMake 项目结构搭建
- [ ] Core 模块（容器、数学、线程）
- [ ] RHI Vulkan 基础（Device、Queue、CommandBuffer）
- [ ] 窗口系统（GLFW 集成）
- [ ] Render Graph 核心
  - [ ] 资源声明与管理
  - [ ] Pass 注册与依赖
  - [ ] 基础执行流程
- [ ] 简单 Triangle 渲染测试

### Phase 2: 资源管理 + Shader 系统
**目标**: 完善资源管线

- [ ] 纹理加载（PNG、JPEG）
- [ ] 模型加载（OBJ、GLTF）
- [ ] Shader 运行时编译
- [ ] Shader 反射系统
- [ ] 自动资源屏障管理
- [ ] 资源热重载

### Phase 3: 内置 Pass 实现
**目标**: 实现完整渲染管线

- [ ] Shadow Pass（Shadow Mapping + CSM）
- [ ] G-Buffer Pass
- [ ] Deferred Lighting Pass
- [ ] Forward Pass
- [ ] 多队列调度
- [ ] 资源别名优化

### Phase 4: 后处理栈
**目标**: 图像质量提升

- [ ] Bloom
- [ ] Tone Mapping（ACES、Reinhard）
- [ ] FXAA
- [ ] TAA
- [ ] 动态分辨率

### Phase 5: PBR 材质 + 光照
**目标**: 物理渲染完整支持

- [ ] PBR 材质系统
- [ ] 方向光、点光源、聚光灯
- [ ] IBL（辐照度、预过滤）
- [ ] GPU 时间戳查询

### Phase 6: 调试工具 + ImGui
**目标**: 开发体验优化

- [ ] ImGui 集成
- [ ] 性能统计面板
- [ ] Render Graph 可视化
- [ ] 资源浏览器
- [ ] 材质编辑器基础

---

## 12. 文件结构

```
HappyCat/
├── CMakeLists.txt
├── spec.md
├── README.md
├── scripts/
│   ├── setup_deps_windows.ps1   # Windows 依赖安装脚本
│   └── setup_deps_linux.sh      # Linux 依赖安装脚本
├── third_party/
│   ├── HappyCatDeps.cmake       # 自动生成的 CMake 配置
│   ├── glfw/
│   ├── imgui/
│   ├── glm/
│   ├── spdlog/
│   ├── stb/
│   ├── tinyobjloader/
│   ├── tinygltf/
│   ├── glslang/
│   └── spirv-cross/
│
├── src/
│   ├── Core/
│   │   ├── CorePCH.h
│   │   ├── Memory/
│   │   ├── Container/
│   │   ├── Threading/
│   │   ├── Math/
│   │   └── Utils/
│   │
│   ├── RHI/
│   │   ├── RHITypes.h
│   │   └── Vulkan/
│   │
│   ├── Renderer/
│   │   ├── RenderGraph/
│   │   └── RenderPass/
│   │
│   ├── Resource/
│   │
│   ├── Scene/
│   │
│   ├── PostProcess/
│   │
│   ├── Platform/
│   │
│   └── Engine/
│
├── shaders/
│   ├── common/
│   │   ├── pbr.glsl
│   │   ├── lighting.glsl
│   │   └── utils.glsl
│   ├── shadow/
│   ├── gbuffer/
│   ├── lighting/
│   └── postprocess/
│
├── assets/
│   ├── textures/
│   ├── models/
│   └── environments/
│
└── samples/
    ├── 01_triangle/
    ├── 02_cube/
    ├── 03_pbr/
    └── 04_deferred/
```

---

## 13. 第三方依赖

### 13.1 依赖项清单

| 库名 | 版本 | 用途 | 许可证 |
|------|------|------|--------|
| Vulkan SDK | 1.3.x | 图形 API | Apache 2.0 |
| GLFW | 3.4 | 窗口与输入 | zlib |
| Dear ImGui | docking | 调试 UI | MIT |
| glm | 1.0.1 | 数学库 | MIT |
| spdlog | 1.14.1 | 日志系统 | MIT |
| stb | latest | 图像加载/保存 | MIT |
| tinyobjloader | v2.0.0rc13 | OBJ 模型加载 | MIT |
| tinygltf | v2.9.2 | GLTF 模型加载 | MIT |
| glslang | 14.3.0 | GLSL -> SPIR-V 编译 | BSD-3 / Apache 2.0 |
| SPIRV-Cross | vulkan-sdk-1.3.290.0 | SPIR-V 反射 | Apache 2.0 |

### 13.2 仓库地址

| 库名 | 仓库地址 |
|------|----------|
| GLFW | https://github.com/glfw/glfw |
| Dear ImGui | https://github.com/ocornut/imgui |
| glm | https://github.com/g-truc/glm |
| spdlog | https://github.com/gabime/spdlog |
| stb | https://github.com/nothings/stb |
| tinyobjloader | https://github.com/tinyobjloader/tinyobjloader |
| tinygltf | https://github.com/syoyo/tinygltf |
| glslang | https://github.com/KhronosGroup/glslang |
| SPIRV-Cross | https://github.com/KhronosGroup/SPIRV-Cross |

### 13.3 自动化脚本

- **Windows**: `scripts/setup_deps_windows.ps1`
- **Linux**: `scripts/setup_deps_linux.sh`

**前置要求**:
- Git
- CMake 3.20+
- Vulkan SDK 1.3+ (已安装)

---

## 14. 后续迭代计划

### v1.1
- [ ] 性能分析工具集成 (Tracy)
- [ ] 截图/录屏功能
- [ ] 高 DPI 支持

### v1.2
- [ ] VSM/ESM 阴影
- [ ] SSAO
- [ ] SSR

### v1.3
- [ ] GPU Driven Rendering
- [ ] Mesh Shader 支持
- [ ] Virtual Texture

### v2.0
- [ ] Ray Tracing 支持
- [ ] Hybrid Rendering (光栅化 + 光线追踪)

---

*文档结束*
