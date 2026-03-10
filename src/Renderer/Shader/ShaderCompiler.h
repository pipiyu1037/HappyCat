#pragma once

#include "Core/Utils/Types.h"
#include "RHI/RHITypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace happycat {

// Compile result
struct ShaderCompileResult {
    bool success = false;
    std::vector<u32> spirv;
    std::string errorMessage;
};

// Shader source info
struct ShaderSource {
    std::string sourceCode;
    std::string filePath;  // For error messages
    ShaderStage stage;
};

// Shader compiler using glslang
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    // Non-copyable
    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    // Initialize the compiler (must call before compilation)
    bool Initialize();

    // Compile GLSL source to SPIR-V
    ShaderCompileResult Compile(const ShaderSource& source);

    // Compile from file
    ShaderCompileResult CompileFromFile(const std::string& filePath, ShaderStage stage);

    // Convenience: compile vertex shader from file
    ShaderCompileResult CompileVertexShader(const std::string& filePath);

    // Convenience: compile fragment shader from file
    ShaderCompileResult CompileFragmentShader(const std::string& filePath);

    // Check if a file is a SPIR-V binary
    static bool IsSpirVFile(const std::string& filePath);

    // Load SPIR-V binary from file
    static std::vector<u32> LoadSpirV(const std::string& filePath);

private:
    // Determine shader stage from file extension
    static ShaderStage GetStageFromExtension(const std::string& ext);

    bool m_Initialized = false;
};

// Global shader compiler instance
class ShaderCompilerManager {
public:
    static ShaderCompiler* Get();
    static void Initialize();
    static void Shutdown();

private:
    static std::unique_ptr<ShaderCompiler> s_Compiler;
};

} // namespace happycat
