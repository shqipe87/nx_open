# Wave Analytics Plugin with OpenCV

A production-ready video analytics plugin for Hanwha Wave VMS (formerly Nx Witness) using OpenCV and Wave SDK 6.0. Provides advanced real-time video analytics including object detection, tracking, behavioral analytics, queue management, and custom business rules.

## Features

### Object Detection & Tracking
- **YOLOv8** and **MobileNet SSD** support for object detection
- **SORT/DeepSORT**-based multi-object tracking with Kalman filtering
- Consistent object IDs across frames
- Support for 80+ COCO classes (person, vehicle, animal, etc.)
- GPU acceleration via CUDA (optional)

### Behavioral Analytics
1. **Loitering Detection**: Detect stationary objects with configurable time thresholds per zone
2. **Line Crossing**: Define virtual tripwires with directional detection (A→B vs B→A)
3. **Intrusion Detection**: Alert on entry to restricted zones with time-based activation
4. **Fall Detection**: Detect person falls using orientation and height change analysis
5. **Aggressive Behavior**: Identify rapid movements and sudden direction changes

### Queue Management Analytics
- Track customer journey through queue zones (entrance → waiting → service → exit)
- Real-time metrics:
  - Current queue length
  - Average wait time (30min/hour/day windows)
  - Service rate (customers/hour)
  - Abandonment rate
- Alerts for queue length, wait time, and abandonment thresholds

### Crowd Density Analysis
- Real-time crowd counting per zone
- Configurable density levels (low/medium/high/critical)
- Density change events
- Heatmap generation for visualization

### Business Rules Engine
- JSON-based rule configuration
- Expression-based conditions supporting:
  - Object type, speed, zone membership
  - Time constraints (hour ranges)
  - AND/OR logic
  - Custom thresholds
- Example rules:
  - "No vehicles in pedestrian zone"
  - "Person in restricted area after hours"
  - "Loitering near ATM at night"

## Requirements

### Build Requirements
- CMake 3.14+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- OpenCV 4.x
- Wave SDK 6.0 (included in nx_open repository)
- (Optional) CUDA Toolkit 11.0+ for GPU acceleration

### Runtime Requirements
- Hanwha Wave VMS Server
- OpenCV 4.x libraries
- AI model files (YOLOv8 ONNX or MobileNet Caffe)
- (Optional) CUDA runtime for GPU acceleration

## Installation

### 1. Download AI Models

Download YOLOv8 ONNX model:
```bash
# Create models directory
mkdir -p /opt/wave_analytics/models

# Download YOLOv8n (nano) - fastest, recommended for real-time
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.onnx \
  -O /opt/wave_analytics/models/yolov8n.onnx

# Or download YOLOv8s (small) - better accuracy
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8s.onnx \
  -O /opt/wave_analytics/models/yolov8s.onnx
```

### 2. Build Plugin

#### Option A: Build within Wave repository

```bash
cd /path/to/nx_open

# Configure build
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DOpenCV_DIR=/path/to/opencv/build

# Build plugin
cmake --build build --target wave_analytics_plugin_opencv -j$(nproc)

# Plugin will be in: build/bin/plugins/wave_analytics_plugin_opencv.so
```

#### Option B: Build standalone

```bash
cd vms/server/plugins/analytics/wave_analytics_plugin_opencv

# Configure
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DNX_SDK_DIR=../../../libs/nx_sdk \
  -DOpenCV_DIR=/path/to/opencv/build

# Build
cmake --build build -j$(nproc)

# Install
sudo cmake --install build
```

### 3. Install Plugin

```bash
# Copy plugin to Wave plugins directory
sudo cp build/bin/plugins/wave_analytics_plugin_opencv.so \
  /opt/networkoptix/mediaserver/bin/plugins/

# Copy configuration (optional)
sudo mkdir -p /etc/wave_analytics
sudo cp config/*.json /etc/wave_analytics/

# Restart Wave Server
sudo systemctl restart networkoptix-mediaserver
```

## Configuration

### Plugin Settings (Web UI)

After installing, configure the plugin in Wave Client:
1. Open Settings → Plugins
2. Select "Wave Analytics Plugin with OpenCV"
3. Configure:
   - **Detection Model**: yolov8n, yolov8s, yolov8m, or mobilenet_ssd
   - **Model Path**: `/opt/wave_analytics/models/yolov8n.onnx`
   - **Confidence Threshold**: 0.3 - 0.8 (default: 0.5)
   - **Use GPU**: Enable CUDA acceleration
   - **Frame Skip Rate**: Process every Nth frame (1-10)
   - **Analytics Features**: Enable/disable specific features

### JSON Configuration Files

Advanced configuration via JSON files (see `config/` directory):

#### Default Configuration (`config.json`)
```json
{
  "detection": {
    "model": "yolov8n",
    "confidence": 0.5,
    "useGPU": true
  },
  "zones": [...],
  "tripLines": [...],
  "rules": [...]
}
```

#### Retail Queue Configuration (`retail_queue.json`)
Optimized for retail checkout queue monitoring with multiple queue zones.

#### Security Monitoring (`security_monitoring.json`)
Configured for perimeter security with intrusion detection and loitering alerts.

### Defining Zones

Zones are polygonal areas for analytics:

```json
{
  "name": "restricted_area",
  "type": "restricted",  // or: queue_entrance, queue_waiting, queue_service
  "coordinates": [[x1,y1], [x2,y2], [x3,y3], [x4,y4]],
  "loiteringThresholdSeconds": 30,
  "maxOccupancy": 10,
  "startHour": 18,  // Active hours
  "endHour": 6
}
```

**Zone Types:**
- `queue_entrance`: Entry point for queue tracking
- `queue_waiting`: Waiting area
- `queue_service`: Service/checkout counter
- `queue_exit`: Exit area
- `restricted`: Restricted access zone
- `intrusion`: Intrusion detection zone

### Defining Trip Lines

Virtual lines for counting crossings:

```json
{
  "name": "entrance_line",
  "start": [x1, y1],
  "end": [x2, y2],
  "bidirectional": true,
  "directionAtoB": "entering",
  "directionBtoA": "exiting"
}
```

### Business Rules

Create custom rules with conditions and actions:

```json
{
  "name": "No trucks after 6pm",
  "condition": "object.type == 'truck' && time.hour >= 18",
  "action": "alert",
  "priority": "high",
  "enabled": true
}
```

**Supported Conditions:**
- `object.type`: Object class name
- `object.speed`: Object speed in pixels/frame
- `time.hour`: Current hour (0-23)
- `zone.{name}`: Boolean, true if object in zone
- Operators: `==`, `!=`, `>`, `<`, `>=`, `<=`
- Logic: `&&` (AND), `||` (OR)

## Events Generated

The plugin generates the following event types in Wave VMS:

| Event Type | Description | Severity |
|------------|-------------|----------|
| `New Object Detected` | New tracked object appears | Low |
| `Loitering Detected` | Object stationary beyond threshold | Medium |
| `Line Crossing` | Object crosses virtual tripwire | Low |
| `Zone Intrusion` | Object enters restricted zone | High |
| `Fall Detected` | Person fall detected | Critical |
| `Aggressive Behavior` | Rapid movements detected | High |
| `Crowd Density Change` | Zone density level changed | Low-Critical |
| `Queue Length Exceeded` | Queue length over threshold | High |
| `Long Wait Time` | Average wait time exceeded | Medium |
| `Customer Abandoned Queue` | Customer left before service | Medium |
| `Business Rule Violation` | Custom rule triggered | Varies |

Events appear in Wave's Events panel with:
- Timestamp and duration
- Bounding box overlay
- Object track ID
- Custom attributes
- Severity level

## Performance Optimization

### Frame Skip Rate
Process every Nth frame to reduce CPU/GPU load:
```json
"frameSkipRate": 2  // Process every 2nd frame
```

### Model Selection
- **yolov8n**: Fastest, 15-25 FPS on 1080p (CPU)
- **yolov8s**: Balanced, 10-15 FPS on 1080p (CPU)
- **yolov8m**: Best accuracy, 5-10 FPS (requires GPU)
- **mobilenet_ssd**: Fast, lower accuracy

### GPU Acceleration
Enable CUDA for 3-5x performance boost:
```json
"useGPU": true
```

Requirements:
- NVIDIA GPU with CUDA support
- CUDA Toolkit 11.0+
- OpenCV compiled with CUDA support

### Recommended Settings by Resolution

**1080p (1920x1080):**
- Model: yolov8n
- Frame Skip: 1-2
- Confidence: 0.5
- Expected: 15-20 FPS (CPU), 30-45 FPS (GPU)

**4K (3840x2160):**
- Model: yolov8n
- Frame Skip: 3-4
- Confidence: 0.5
- Expected: 5-8 FPS (CPU), 15-25 FPS (GPU)

## Troubleshooting

### Plugin Not Loading
```bash
# Check plugin file exists
ls -l /opt/networkoptix/mediaserver/bin/plugins/wave_analytics_plugin_opencv.so

# Check Wave Server logs
sudo journalctl -u networkoptix-mediaserver -f

# Verify dependencies
ldd /opt/networkoptix/mediaserver/bin/plugins/wave_analytics_plugin_opencv.so
```

### Missing OpenCV Libraries
```bash
# Install OpenCV (Ubuntu/Debian)
sudo apt-get install libopencv-dev libopencv-contrib-dev

# Or build from source
git clone https://github.com/opencv/opencv.git
cd opencv
cmake -B build -DWITH_CUDA=ON -DCUDA_ARCH_BIN=7.5
cmake --build build -j$(nproc)
sudo cmake --install build
```

### Model File Not Found
```bash
# Verify model path in configuration matches actual file location
ls -l /opt/wave_analytics/models/yolov8n.onnx

# Update config if needed
sudo nano /etc/wave_analytics/config.json
```

### Poor Detection Performance
1. **Lower confidence threshold**: Try 0.3-0.4 for more detections
2. **Use better model**: Switch from yolov8n to yolov8s
3. **Check lighting**: Ensure adequate illumination
4. **Verify GPU**: Check if CUDA is actually being used

### High CPU/GPU Usage
1. **Increase frame skip**: Process every 2-3 frames instead of every frame
2. **Use smaller model**: Switch to yolov8n or mobilenet_ssd
3. **Reduce resolution**: Process at 720p instead of 1080p
4. **Limit cameras**: Don't enable on all cameras simultaneously

## Docker Support

Build and run in Docker container:

```bash
cd docker

# Build image
docker-compose build

# Run tests
docker-compose run analytics_plugin /bin/bash -c "cd build && ctest"

# Interactive development
docker-compose run analytics_plugin /bin/bash
```

See `docker/README.md` for details.

## Development

### Project Structure
```
wave_analytics_plugin_opencv/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── plugin/
│   └── plugin.cpp              # Plugin entry point
├── src/nx/vms_server_plugins/analytics/wave_opencv/
│   ├── integration.{h,cpp}     # IIntegration implementation
│   ├── engine.{h,cpp}          # IEngine implementation
│   ├── device_agent.{h,cpp}    # IDeviceAgent implementation
│   ├── types.h                 # Common types and structures
│   ├── object_detector.{h,cpp} # YOLO/MobileNet detector
│   ├── object_tracker.{h,cpp}  # SORT tracker with Kalman filter
│   ├── behavioral_analyzer.{h,cpp}  # Behavioral analytics
│   ├── crowd_analyzer.{h,cpp}       # Crowd density
│   ├── queue_manager.{h,cpp}        # Queue analytics
│   └── rules_engine.{h,cpp}         # Business rules
├── config/                     # Configuration files
│   ├── config.json
│   ├── retail_queue.json
│   └── security_monitoring.json
├── models/                     # AI model files (not in repo)
├── docker/                     # Docker build files
└── tests/                      # Unit tests
```

### Adding New Analytics

1. **Create analyzer class** inheriting from base analyzer
2. **Add configuration** to `PluginConfiguration` in `types.h`
3. **Integrate in DeviceAgent** (`device_agent.cpp`)
4. **Add event types** to manifest (`engine.cpp`)
5. **Update documentation**

### Testing

```bash
# Build with tests
cmake -B build -DBUILD_TESTING=ON
cmake --build build

# Run tests
cd build
ctest --output-on-failure
```

## API Reference

See code documentation in header files:
- `types.h`: Data structures and common types
- `object_detector.h`: Detection API
- `object_tracker.h`: Tracking API
- `behavioral_analyzer.h`: Behavioral analytics API
- `crowd_analyzer.h`: Crowd analysis API
- `queue_manager.h`: Queue management API
- `rules_engine.h`: Rules engine API

## License

Copyright 2024. All Rights Reserved.

This plugin is provided as-is for use with Hanwha Wave VMS.

## Support

For issues and questions:
- Check `TROUBLESHOOTING.md`
- Review Wave SDK documentation: `/vms/libs/nx_sdk/src/nx/sdk/analytics/`
- Open issue on GitHub repository

## Acknowledgments

- Built on Hanwha Wave VMS SDK 6.0
- Uses OpenCV for computer vision
- YOLOv8 models from Ultralytics
- SORT tracking algorithm

## Version History

**1.0.0** (2024)
- Initial release
- YOLOv8 and MobileNet SSD support
- SORT-based tracking
- Behavioral analytics (loitering, line crossing, intrusion, fall, aggressive behavior)
- Queue management
- Crowd density analysis
- Business rules engine
- GPU acceleration support
