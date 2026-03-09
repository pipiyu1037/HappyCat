# HappyCat Engine - Windows Dependencies Setup Script
# Usage: Run in PowerShell
# Requirements: Git, CMake 3.20+
#
# Note: glslang and SPIRV-Cross are provided by Vulkan SDK
# This script only downloads dependencies not included in Vulkan SDK

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
# Note: glslang and SPIRV-Cross are now provided by Vulkan SDK
$Versions = @{
    GLFW            = "3.4"              # git tag: 3.4
    ImGui           = "docking"          # branch name
    GLM             = "1.0.1"            # git tag: 1.0.1
    SPDLOG          = "1.14.1"           # git tag: v1.14.1
    TINYOBJLOADER   = "v2.0.0rc13"       # git tag
    TINYGLTF        = "v2.9.2"           # git tag
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

    # Check Vulkan SDK
    if (-not $env:VULKAN_SDK) {
        Write-Warning "VULKAN_SDK environment variable not set!"
        Write-Info "Please install Vulkan SDK from https://vulkan.lunarg.com/"
    } else {
        Write-Success "Vulkan SDK: $($env:VULKAN_SDK)"
    }

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

# Main
function Main {
    Write-Host ""
    Write-Host "======================================" -ForegroundColor Magenta
    Write-Host "  HappyCat - Windows Dependencies" -ForegroundColor Magenta
    Write-Host "======================================" -ForegroundColor Magenta
    Write-Host ""
    Write-Host "Note: glslang and SPIRV-Cross are provided by Vulkan SDK" -ForegroundColor Yellow
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

    Write-Host ""
    Write-Host "======================================" -ForegroundColor Green
    Write-Host "  Done! All dependencies installed." -ForegroundColor Green
    Write-Host "======================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "glslang and SPIRV-Cross will be used from Vulkan SDK" -ForegroundColor Cyan
    Write-Host ""
}

Main
