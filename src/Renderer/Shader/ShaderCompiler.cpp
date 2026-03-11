#include "ShaderCompiler.h"
#include "Core/Utils/Logger.h"

#include <fstream>
#include <sstream>

// shaderc headers
#include <shaderc/shaderc.hpp>

namespace happycat {

// Global compiler instance
std::unique_ptr<ShaderCompiler> ShaderCompilerManager::s_Compiler;

ShaderCompiler* ShaderCompilerManager::Get() {
    return s_Compiler.get();
}

void ShaderCompilerManager::Initialize() {
    if (!s_Compiler) {
        s_Compiler = std::make_unique<ShaderCompiler>();
        if (!s_Compiler->Initialize()) {
            HC_CORE_ERROR("Failed to initialize shader compiler");
            s_Compiler.reset();
        }
    }
}

void ShaderCompilerManager::Shutdown() {
    s_Compiler.reset();
}

ShaderCompiler::ShaderCompiler() {
}

ShaderCompiler::~ShaderCompiler() {
    m_Initialized = false;
}

bool ShaderCompiler::Initialize() {
    if (m_Initialized) {
        return true;
    }

    m_Initialized = true;
    HC_CORE_INFO("Shader compiler initialized (using shaderc)");
    return true;
}

shaderc_shader_kind GetShadercKind(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:    return shaderc_vertex_shader;
        case ShaderStage::Fragment:  return shaderc_fragment_shader;
        case ShaderStage::Compute:   return shaderc_compute_shader;
        case ShaderStage::Geometry:  return shaderc_geometry_shader;
        default:                     return shaderc_vertex_shader;
    }
}

ShaderCompileResult ShaderCompiler::Compile(const ShaderSource& source) {
    ShaderCompileResult result;

    if (!m_Initialized) {
        result.errorMessage = "Shader compiler not initialized";
        return result;
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // Set target environment to Vulkan SPIR-V
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc_shader_kind kind = GetShadercKind(source.stage);
    shaderc::SpvCompilationResult spvResult = compiler.CompileGlslToSpv(
        source.sourceCode, kind, source.filePath.c_str(), options);

    if (spvResult.GetCompilationStatus() != shaderc_compilation_status_success) {
        result.errorMessage = "Shader compilation error in " + source.filePath + ":\n" +
                              spvResult.GetErrorMessage();
        HC_CORE_ERROR("{0}", result.errorMessage);
        return result;
    }

    result.spirv.assign(spvResult.cbegin(), spvResult.cend());
    result.success = true;
    return result;
}

ShaderCompileResult ShaderCompiler::CompileFromFile(const std::string& filePath, ShaderStage stage) {
    ShaderCompileResult result;

    // Read file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.errorMessage = "Failed to open shader file: " + filePath;
        HC_CORE_ERROR("{0}", result.errorMessage);
        return result;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string sourceCode = oss.str();

    ShaderSource source;
    source.sourceCode = sourceCode;
    source.filePath = filePath;
    source.stage = stage;

    return Compile(source);
}

ShaderCompileResult ShaderCompiler::CompileVertexShader(const std::string& filePath) {
    return CompileFromFile(filePath, ShaderStage::Vertex);
}

ShaderCompileResult ShaderCompiler::CompileFragmentShader(const std::string& filePath) {
    return CompileFromFile(filePath, ShaderStage::Fragment);
}

bool ShaderCompiler::IsSpirVFile(const std::string& filePath) {
    // Check extension
    if (filePath.size() >= 4) {
        std::string ext = filePath.substr(filePath.size() - 4);
        if (ext == ".spv") return true;
    }
    return false;
}

std::vector<u32> ShaderCompiler::LoadSpirV(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        // Silent return - caller will decide whether to compile from source
        return {};
    }

    size_t size = file.tellg();
    file.seekg(0);

    // SPIR-V must be aligned to 4 bytes
    if (size % 4 != 0) {
        HC_CORE_WARN("SPIR-V file size not aligned: {0}", filePath);
        return {};
    }

    std::vector<u32> spirv(size / 4);
    file.read(reinterpret_cast<char*>(spirv.data()), size);

    return spirv;
}

ShaderStage ShaderCompiler::GetStageFromExtension(const std::string& ext) {
    if (ext == "vert") return ShaderStage::Vertex;
    if (ext == "frag") return ShaderStage::Fragment;
    if (ext == "comp") return ShaderStage::Compute;
    if (ext == "geom") return ShaderStage::Geometry;
    return ShaderStage::Vertex; // Default
}

} // namespace happycat
