#!/bin/bash
# Build script for Wave Analytics Plugin with OpenCV

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Wave Analytics Plugin Build Script ===${NC}"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: CMakeLists.txt not found. Run this script from the plugin directory.${NC}"
    exit 1
fi

# Parse arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
INSTALL=false
GPU_SUPPORT=true

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        --no-gpu)
            GPU_SUPPORT=false
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug     Build in Debug mode (default: Release)"
            echo "  -c, --clean     Clean build directory before building"
            echo "  -i, --install   Install plugin after building"
            echo "  --no-gpu        Build without GPU support"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Check for OpenCV
echo -e "${YELLOW}Checking for OpenCV...${NC}"
if ! pkg-config --exists opencv4; then
    echo -e "${RED}Error: OpenCV not found. Please install OpenCV 4.x${NC}"
    echo "Ubuntu/Debian: sudo apt-get install libopencv-dev"
    echo "Or build from source: https://opencv.org/"
    exit 1
fi

OPENCV_VERSION=$(pkg-config --modversion opencv4)
echo -e "${GREEN}Found OpenCV ${OPENCV_VERSION}${NC}"

# Check for CUDA (if GPU support requested)
if [ "$GPU_SUPPORT" = true ]; then
    echo -e "${YELLOW}Checking for CUDA...${NC}"
    if command -v nvidia-smi &> /dev/null; then
        echo -e "${GREEN}CUDA GPU detected${NC}"
        nvidia-smi --query-gpu=name --format=csv,noheader
    else
        echo -e "${YELLOW}Warning: CUDA not found. Building with CPU support only.${NC}"
        echo "To enable GPU support, install CUDA toolkit: https://developer.nvidia.com/cuda-downloads"
    fi
fi

# Configure
echo -e "${YELLOW}Configuring build (${BUILD_TYPE})...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo -e "${YELLOW}Building plugin...${NC}"
cmake --build . -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"

    # Show plugin location
    if [ -f "bin/plugins/wave_analytics_plugin_opencv.so" ]; then
        PLUGIN_PATH="$(pwd)/bin/plugins/wave_analytics_plugin_opencv.so"
        echo -e "${GREEN}Plugin: ${PLUGIN_PATH}${NC}"

        # Show file size
        SIZE=$(du -h "${PLUGIN_PATH}" | cut -f1)
        echo -e "${GREEN}Size: ${SIZE}${NC}"
    else
        echo -e "${YELLOW}Warning: Plugin file not found in expected location${NC}"
    fi

    # Install if requested
    if [ "$INSTALL" = true ]; then
        echo -e "${YELLOW}Installing plugin...${NC}"

        # Check for Wave server
        if [ -d "/opt/networkoptix/mediaserver" ]; then
            INSTALL_DIR="/opt/networkoptix/mediaserver/bin/plugins"
        elif [ -d "/opt/networkoptix-metavms/mediaserver" ]; then
            INSTALL_DIR="/opt/networkoptix-metavms/mediaserver/bin/plugins"
        else
            echo -e "${RED}Error: Wave/Nx server installation not found${NC}"
            exit 1
        fi

        echo -e "${YELLOW}Installing to: ${INSTALL_DIR}${NC}"
        sudo mkdir -p "${INSTALL_DIR}"
        sudo cp bin/plugins/wave_analytics_plugin_opencv.so "${INSTALL_DIR}/"
        sudo chmod 755 "${INSTALL_DIR}/wave_analytics_plugin_opencv.so"

        echo -e "${GREEN}Plugin installed successfully!${NC}"
        echo -e "${YELLOW}Restart Wave server to load the plugin:${NC}"
        echo "  sudo systemctl restart networkoptix-mediaserver"
    else
        echo ""
        echo -e "${YELLOW}To install the plugin, run:${NC}"
        echo "  sudo cp bin/plugins/wave_analytics_plugin_opencv.so /opt/networkoptix/mediaserver/bin/plugins/"
        echo "  sudo systemctl restart networkoptix-mediaserver"
        echo ""
        echo -e "${YELLOW}Or rebuild with --install flag:${NC}"
        echo "  ./build.sh --install"
    fi
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
