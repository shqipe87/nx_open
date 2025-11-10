FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libopencv-dev \
    libopencv-contrib-dev \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install CUDA (optional - comment out if not needed)
# RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.0-1_all.deb && \
#     dpkg -i cuda-keyring_1.0-1_all.deb && \
#     apt-get update && \
#     apt-get install -y cuda-toolkit-11-8 && \
#     rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy source code
COPY . /workspace/wave_analytics_plugin_opencv

# Download YOLOv8 models
RUN mkdir -p /opt/wave_analytics/models && \
    cd /opt/wave_analytics/models && \
    wget -q https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.onnx || \
    echo "Warning: Could not download YOLOv8n model. Please download manually."

# Build plugin
WORKDIR /workspace/wave_analytics_plugin_opencv
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# Set entrypoint
ENTRYPOINT ["/bin/bash"]
