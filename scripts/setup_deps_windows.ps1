# HappyCat Engine - Windows Dependencies Setup Script
# Usage: Run in PowerShell
# Requirements: Git, CMake 3.20+

param(
    [string]$TargetDir = "..\third_party",
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-Info { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warning { param($msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }

# Dependency versions
$Versions = @{
    GLFW            = "3.4"              # git tag: 3.4
    ImGui           = "docking"          # branch name
    GLM             = "1.0.1"            # git tag: 1.0.1
    SPDLOG          = "1.14.1"           # git tag: v1.14.1
    TINYOBJLOADER   = "v2.0.0rc13"       # git tag
    TINYGLTF        = "v2.9.2"           # git tag
    GLSLANG         = "14.3.0"           # git tag
    SPIRVCROSS      = "vulkan-sdk-1.3.290.0"  # git tag
}

# Check prerequisites
function Check-Prerequisites {
    Write-Info "Checking prerequisites..."

    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        Write-Host "[ERROR] Git not found!" -ForegroundColor Red
        exit 1
    }
    Write-Success "Git: $(git --version)"

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Host "[ERROR] CMake not found!" -ForegroundColor Red
        exit 1
    }
    Write-Success "CMake: $(cmake --version | Select-Object -First 1)"

    Write-Host ""
}

# Create directory structure
function Initialize-Directories {
    param($baseDir)

    Write-Info "Creating directory structure..."

    if ($Clean -and (Test-Path $baseDir)) {
        Remove-Item -Recurse -Force $baseDir
    }

    if (-not (Test-Path $baseDir)) {
        New-Item -ItemType Directory -Path $baseDir -Force | Out-Null
    }

    Write-Success "Directory structure created!"
    Write-Host ""
}

# Clone or update a git repository
function Git-Clone-OrUpdate {
    param([string]$Url, [string]$Target, [string]$Branch, [string]$Name)

    $isGitRepo = Test-Path (Join-Path $Target ".git")

    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'

    try {
        if ($isGitRepo) {
            Write-Info "Updating $Name..."
            Push-Location $Target
            git fetch origin --tags 2>&1 | Out-Null
            git checkout $Branch 2>&1 | Out-Null
            git pull origin $Branch 2>&1 | Out-Null
            Pop-Location
        } else {
            if (Test-Path $Target) {
                Remove-Item -Recurse -Force $Target
            }
            Write-Info "Cloning $Name..."
            git clone $Url $Target 2>&1 | Out-Null
            Push-Location $Target
            git checkout $Branch 2>&1 | Out-Null
            Pop-Location
        }
        Write-Success "$Name ready!"
    } finally {
        $ErrorActionPreference = $prevEAP
    }
}

# Download single file with retry
function Download-File {
    param([string]$Url, [string]$Output)

    $fileName = Split-Path $Output -Leaf
    Write-Info "Downloading $fileName..."

    $maxRetries = 3
    $retry = 0
    $success = $false

    while (-not $success -and $retry -lt $maxRetries) {
        try {
            if ($retry -gt 0) {
                Write-Info "Retry $retry/$maxRetries..."
                Start-Sleep -Seconds 2
            }
            Invoke-WebRequest -Uri $Url -OutFile $Output -UseBasicParsing -TimeoutSec 60
            $success = $true
        } catch {
            $retry++
        }
    }

    if ($success) {
        Write-Success "Downloaded: $fileName"
    } else {
        Write-Warning "Failed to download $fileName, trying alternative..."
        # Try with curl if available
        if (Get-Command curl -ErrorAction SilentlyContinue) {
            curl -L -o $Output $Url 2>&1 | Out-Null
            if (Test-Path $Output) {
                Write-Success "Downloaded via curl: $fileName"
                return
            }
        }
        throw "Failed to download $fileName"
    }
}

# Setup GLFW
function Setup-GLFW { param($baseDir)
    Git-Clone-OrUpdate -Url "https://github.com/glfw/glfw.git" -Target "$baseDir/glfw" -Branch $Versions.GLFW -Name "GLFW"
}

# Setup Dear ImGui
function Setup-ImGui { param($baseDir)
    Git-Clone-OrUpdate -Url "https://github.com/ocornut/imgui.git" -Target "$baseDir/imgui" -Branch $Versions.ImGui -Name "ImGui"
}

# Setup GLM
function Setup-GLM { param($baseDir)
    Git-Clone-OrUpdate -Url "https://github.com/g-truc/glm.git" -Target "$baseDir/glm" -Branch $Versions.GLM -Name "GLM"
}

# Setup spdlog
function Setup-Spdlog { param($baseDir)
    Git-Clone-OrUpdate -Url "https://github.com/gabime/spdlog.git" -Target "$baseDir/spdlog" -Branch "v$($Versions.SPDLOG)" -Name "spdlog"
}

# Setup stb libraries
function Setup-STB { param($baseDir)
    $target = "$baseDir/stb"
    New-Item -ItemType Directory -Path $target -Force | Out-Null

    @("stb_image.h", "stb_image_write.h") | ForEach-Object {
        Download-File -Url "https://raw.githubusercontent.com/nothings/stb/master/$_" -Output "$target/$_"
    }
}

# Setup tinyobjloader
function Setup-TinyObjLoader { param($baseDir)
    Git-Clone-OrUpdate -Url "https://github.com/tinyobjloader/tinyobjloader.git" -Target "$baseDir/tinyobjloader" -Branch $Versions.TINYOBJLOADER -Name "tinyobjloader"
}

# Setup tinygltf
function Setup-TinyGLTF { param($baseDir)
    Git-Clone-OrUpdate -Url "https://github.com/syoyo/tinygltf.git" -Target "$baseDir/tinygltf" -Branch $Versions.TINYGLTF -Name "tinygltf"
}

# Setup glslang
function Setup-Glslang { param($baseDir)
    $target = "$baseDir/glslang"
    Git-Clone-OrUpdate -Url "https://github.com/KhronosGroup/glslang.git" -Target $target -Branch $Versions.GLSLANG -Name "glslang"

    Push-Location $target
    git submodule update --init --recursive
    Pop-Location

    Write-Info "Building glslang..."
    $buildDir = "$target/build"
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

    Push-Location $buildDir
    cmake .. -DCMAKE_INSTALL_PREFIX="$target/install" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DENABLE_OPT=OFF -DENABLE_GLSLANG_BIN=OFF
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    Pop-Location
    Write-Success "glslang built!"
}

# Setup SPIRV-Cross
function Setup-SPIRVCross { param($baseDir)
    $target = "$baseDir/spirv-cross"
    Git-Clone-OrUpdate -Url "https://github.com/KhronosGroup/SPIRV-Cross.git" -Target $target -Branch $Versions.SPIRVCROSS -Name "SPIRV-Cross"

    Write-Info "Building SPIRV-Cross..."
    $buildDir = "$target/build"
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

    Push-Location $buildDir
    cmake .. -DCMAKE_INSTALL_PREFIX="$target/install" -DCMAKE_BUILD_TYPE=Release -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_STATIC=ON -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_ENABLE_GLSL=ON -DSPIRV_CROSS_ENABLE_HLSL=OFF -DSPIRV_CROSS_ENABLE_MSL=OFF -DSPIRV_CROSS_ENABLE_CPP=OFF -DSPIRV_CROSS_ENABLE_REFLECT=ON
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    Pop-Location
    Write-Success "SPIRV-Cross built!"
}

# Generate CMake config
function Generate-CMakeConfig { param($baseDir)
    Write-Info "Generating CMake configuration..."

    $content = @'
# HappyCat Engine - Third Party Dependencies Configuration
set(HAPPYCAT_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

find_package(Vulkan REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/glfw)

# ImGui
add_library(imgui STATIC
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${HAPPYCAT_THIRD_PARTY_DIR}/imgui ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# GLM
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/glm)

# spdlog
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/spdlog)

# stb (header-only)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${HAPPYCAT_THIRD_PARTY_DIR}/stb)

# tinyobjloader
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/tinyobjloader)

# tinygltf (header-only)
add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE ${HAPPYCAT_THIRD_PARTY_DIR}/tinygltf)

# glslang
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/glslang)

# SPIRV-Cross
find_package(spirv_cross REQUIRED PATHS ${HAPPYCAT_THIRD_PARTY_DIR}/spirv-cross/install NO_DEFAULT_PATH)

# Interface library
add_library(HappyCatDeps INTERFACE)
target_link_libraries(HappyCatDeps INTERFACE glfw imgui glm::glm spdlog::spdlog stb tinyobjloader tinygltf SPIRV glslang MachineIndependent GenericCodeGen spirv-cross-core spirv-cross-glsl spirv-cross-reflect Vulkan::Vulkan)
'@

    $content | Out-File -FilePath "$baseDir/HappyCatDeps.cmake" -Encoding UTF8
    Write-Success "Generated HappyCatDeps.cmake"
}

# Main
function Main {
    Write-Host ""
    Write-Host "======================================" -ForegroundColor Magenta
    Write-Host "  HappyCat - Windows Dependencies" -ForegroundColor Magenta
    Write-Host "======================================" -ForegroundColor Magenta
    Write-Host ""

    $baseDir = Join-Path $PSScriptRoot $TargetDir

    Check-Prerequisites
    Initialize-Directories -baseDir $baseDir

    Setup-GLFW -baseDir $baseDir
    Setup-ImGui -baseDir $baseDir
    Setup-GLM -baseDir $baseDir
    Setup-Spdlog -baseDir $baseDir
    Setup-STB -baseDir $baseDir
    Setup-TinyObjLoader -baseDir $baseDir
    Setup-TinyGLTF -baseDir $baseDir
    Setup-Glslang -baseDir $baseDir
    Setup-SPIRVCross -baseDir $baseDir

    Generate-CMakeConfig -baseDir $baseDir

    Write-Host ""
    Write-Host "======================================" -ForegroundColor Green
    Write-Host "  Done! All dependencies installed." -ForegroundColor Green
    Write-Host "======================================" -ForegroundColor Green
    Write-Host ""
}

Main
