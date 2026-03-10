#include "ShaderCompiler.h"
#include "Core/Utils/Logger.h"

#include <fstream>
#include <sstream>

// glslang headers
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

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
    if (m_Initialized) {
        glslang::FinalizeProcess();
        m_Initialized = false;
    }
}

bool ShaderCompiler::Initialize() {
    if (m_Initialized) {
        return true;
    }

    if (!glslang::InitializeProcess()) {
        HC_CORE_ERROR("Failed to initialize glslang process");
        return false;
    }

    m_Initialized = true;
    HC_CORE_INFO("Shader compiler initialized");
    return true;
}

EShLanguage GetShLanguage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:    return EShLangVertex;
        case ShaderStage::Fragment:  return EShLangFragment;
        case ShaderStage::Compute:   return EShLangCompute;
        case ShaderStage::Geometry:  return EShLangGeometry;
        default:                         return EShLangVertex;
    }
}

ShaderCompileResult ShaderCompiler::Compile(const ShaderSource& source) {
    ShaderCompileResult result;

    if (!m_Initialized) {
        result.errorMessage = "Shader compiler not initialized";
        return result;
    }

    EShLanguage shLang = GetShLanguage(source.stage);
    glslang::TShader shader(shLang);

    const char* sourceStr = source.sourceCode.c_str();
    const int sourceLen = static_cast<int>(source.sourceCode.length());
    const char* filePath = source.filePath.c_str();

    shader.setStringsWithLengthsAndNames(&sourceStr, &sourceLen, &filePath, 1);

    // Set Vulkan SPIR-V target
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

    // Get default resources
    const TBuiltInResource* resources = GetDefaultResources();

    // Parse
    EShMessages messages = static_cast<EShMessages>(
        EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault
    );

    if (!shader.parse(resources, 450, false, messages)) {
        std::ostringstream oss;
        oss << "Shader parse error in " << source.filePath << ":\n";
        oss << shader.getInfoLog() << "\n";
        oss << shader.getInfoDebugLog();
        result.errorMessage = oss.str();
        HC_CORE_ERROR("{0}", result.errorMessage);
        return result;
    }

    // Link into program
    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        std::ostringstream oss;
        oss << "Shader link error in " << source.filePath << ":\n";
        oss << program.getInfoLog() << "\n";
        oss << program.getInfoDebugLog();
        result.errorMessage = oss.str();
        HC_CORE_ERROR("{0}", result.errorMessage);
        return result;
    }

    // Generate SPIR-V
    spv::SpvBuildLogger spvLogger;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = false;
    spvOptions.disableOptimizer = false;
    spvOptions.optimizeSize = true;

    glslang::GlslangToSpv(*program.getIntermediate(shLang), result.spirv, &spvLogger, &spvOptions);

    std::string spvMessages = spvLogger.getAllMessages();
    if (!spvMessages.empty()) {
        HC_CORE_WARN("SPIR-V generation warnings: {0}", spvMessages);
    }

    result.success = true;
    HC_CORE_INFO("Compiled shader: {0} ({1} bytes)", source.filePath, result.spirv.size() * 4);
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
        HC_CORE_ERROR("Failed to open SPIR-V file: {0}", filePath);
        return {};
    }

    size_t size = file.tellg();
    file.seekg(0);

    // SPIR-V must be aligned to 4 bytes
    if (size % 4 != 0) {
        HC_CORE_ERROR("SPIR-V file size not aligned: {0}", filePath);
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
