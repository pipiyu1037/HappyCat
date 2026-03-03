#!/bin/bash
# HappyCat Engine - Linux Dependencies Setup Script
# Usage: ./setup_deps_linux.sh [--clean]
# Requirements: Git, CMake 3.20+, build-essential

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

TARGET_DIR="../third_party"
CLEAN=false
NPROC=$(nproc)

# Versions
GLFW_VERSION="3.4"
IMGUI_BRANCH="docking"
GLM_VERSION="1.0.1"
SPDLOG_VERSION="1.14.1"
TINYOBJLOADER_VERSION="v2.0.0rc13"
TINYGLTF_VERSION="v2.9.2"
GLSLANG_VERSION="14.3.0"
SPIRVCROSS_VERSION="vulkan-sdk-1.3.290.0"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean) CLEAN=true; shift ;;
        --help|-h)
            echo "Usage: $0 [--clean]"
            exit 0 ;;
        *) echo -e "${RED}Unknown option: $1${NC}"; exit 1 ;;
    esac
done

log_info() { echo -e "${CYAN}[INFO]${NC} $1"; }
log_ok() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }

# Check prerequisites
check_prereqs() {
    log_info "Checking prerequisites..."

    command -v git >/dev/null || { echo -e "${RED}Git not found!${NC}"; exit 1; }
    log_ok "Git: $(git --version)"

    command -v cmake >/dev/null || { echo -e "${RED}CMake not found!${NC}"; exit 1; }
    log_ok "CMake: $(cmake --version | head -1)"

    command -v g++ >/dev/null || command -v clang++ >/dev/null || { echo -e "${RED}C++ compiler not found!${NC}"; exit 1; }
    log_ok "C++ compiler found"

    echo ""
}

# Create directories
init_dirs() {
    log_info "Creating directory structure..."

    [ "$CLEAN" = true ] && [ -d "$TARGET_DIR" ] && rm -rf "$TARGET_DIR"
    mkdir -p "$TARGET_DIR"

    for dir in glfw imgui glm spdlog stb tinyobjloader tinygltf glslang spirv-cross; do
        mkdir -p "$TARGET_DIR/$dir"
    done

    log_ok "Done!"
    echo ""
}

# Clone or update
git_clone() {
    local url=$1 target=$2 branch=${3:-main} name=$4

    if [ -d "$target" ]; then
        log_info "Updating $name..."
        cd "$target"
        git fetch origin
        git checkout "$branch" 2>/dev/null || true
        git pull origin "$branch"
        cd - > /dev/null
    else
        log_info "Cloning $name..."
        git clone --depth 1 --branch "$branch" "$url" "$target"
    fi
    log_ok "$name ready!"
}

# Download file
download() {
    local url=$1 output=$2
    log_info "Downloading $(basename "$output")..."
    wget -q -O "$output" "$url" || curl -sL -o "$output" "$url"
    log_ok "Downloaded $(basename "$output")"
}

# Setup functions
setup_glfw() {
    git_clone "https://github.com/glfw/glfw.git" "$TARGET_DIR/glfw" "$GLFW_VERSION" "GLFW"
}

setup_imgui() {
    git_clone "https://github.com/ocornut/imgui.git" "$TARGET_DIR/imgui" "$IMGUI_BRANCH" "ImGui"
}

setup_glm() {
    git_clone "https://github.com/g-truc/glm.git" "$TARGET_DIR/glm" "$GLM_VERSION" "GLM"
}

setup_spdlog() {
    git_clone "https://github.com/gabime/spdlog.git" "$TARGET_DIR/spdlog" "v$SPDLOG_VERSION" "spdlog"
}

setup_stb() {
    mkdir -p "$TARGET_DIR/stb"
    for f in stb_image.h stb_image_write.h stb_image_resize2.h; do
        download "https://raw.githubusercontent.com/nothings/stb/master/$f" "$TARGET_DIR/stb/$f"
    done
}

setup_tinyobjloader() {
    git_clone "https://github.com/tinyobjloader/tinyobjloader.git" "$TARGET_DIR/tinyobjloader" "$TINYOBJLOADER_VERSION" "tinyobjloader"
}

setup_tinygltf() {
    git_clone "https://github.com/syoyo/tinygltf.git" "$TARGET_DIR/tinygltf" "$TINYGLTF_VERSION" "tinygltf"
}

setup_glslang() {
    git_clone "https://github.com/KhronosGroup/glslang.git" "$TARGET_DIR/glslang" "$GLSLANG_VERSION" "glslang"

    cd "$TARGET_DIR/glslang"
    git submodule update --init --recursive
    cd - > /dev/null

    log_info "Building glslang..."
    mkdir -p "$TARGET_DIR/glslang/build"
    cd "$TARGET_DIR/glslang/build"
    cmake .. -DCMAKE_INSTALL_PREFIX="$TARGET_DIR/glslang/install" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DENABLE_OPT=OFF -DENABLE_GLSLANG_BIN=OFF
    make -j$NPROC
    make install
    cd - > /dev/null
    log_ok "glslang built!"
}

setup_spirvcross() {
    git_clone "https://github.com/KhronosGroup/SPIRV-Cross.git" "$TARGET_DIR/spirv-cross" "$SPIRVCROSS_VERSION" "SPIRV-Cross"

    log_info "Building SPIRV-Cross..."
    mkdir -p "$TARGET_DIR/spirv-cross/build"
    cd "$TARGET_DIR/spirv-cross/build"
    cmake .. -DCMAKE_INSTALL_PREFIX="$TARGET_DIR/spirv-cross/install" -DCMAKE_BUILD_TYPE=Release -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_STATIC=ON -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_ENABLE_GLSL=ON -DSPIRV_CROSS_ENABLE_HLSL=OFF -DSPIRV_CROSS_ENABLE_MSL=OFF -DSPIRV_CROSS_ENABLE_CPP=OFF -DSPIRV_CROSS_ENABLE_REFLECT=ON
    make -j$NPROC
    make install
    cd - > /dev/null
    log_ok "SPIRV-Cross built!"
}

# Generate CMake config
gen_cmake() {
    log_info "Generating CMake configuration..."

    cat > "$TARGET_DIR/HappyCatDeps.cmake" << 'EOF'
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
EOF

    log_ok "Generated HappyCatDeps.cmake"
}

# Main
main() {
    echo ""
    echo -e "${MAGENTA}======================================${NC}"
    echo -e "${MAGENTA}  HappyCat - Linux Dependencies${NC}"
    echo -e "${MAGENTA}======================================${NC}"
    echo ""

    check_prereqs
    init_dirs

    setup_glfw
    setup_imgui
    setup_glm
    setup_spdlog
    setup_stb
    setup_tinyobjloader
    setup_tinygltf
    setup_glslang
    setup_spirvcross

    gen_cmake

    echo ""
    echo -e "${GREEN}======================================${NC}"
    echo -e "${GREEN}  Done! All dependencies installed.${NC}"
    echo -e "${GREEN}======================================${NC}"
    echo ""
}

main
