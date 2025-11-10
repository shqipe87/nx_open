# AI Models Directory

This directory should contain the AI models used for object detection.

## Downloading Models

### YOLOv8 Models (Recommended)

Download from Ultralytics GitHub releases:

```bash
# YOLOv8n (Nano) - Fastest, recommended for real-time
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.onnx

# YOLOv8s (Small) - Balanced speed/accuracy
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8s.onnx

# YOLOv8m (Medium) - Better accuracy, slower
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8m.onnx

# YOLOv8l (Large) - High accuracy
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8l.onnx

# YOLOv8x (XLarge) - Best accuracy, very slow
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8x.onnx
```

### MobileNet SSD (Alternative)

For MobileNet SSD, you'll need both the model and prototxt files:

```bash
# Download MobileNet SSD v2 (COCO)
wget https://github.com/chuanqi305/MobileNet-SSD/raw/master/mobilenet_iter_73000.caffemodel
wget https://github.com/chuanqi305/MobileNet-SSD/raw/master/deploy.prototxt
```

## Model Specifications

### YOLOv8n (Recommended for CPU)
- **Size**: ~6 MB
- **Speed**: 15-25 FPS on 1080p (CPU), 45-80 FPS (GPU)
- **Accuracy**: Good
- **Use case**: Real-time processing, multiple cameras

### YOLOv8s
- **Size**: ~22 MB
- **Speed**: 10-15 FPS on 1080p (CPU), 30-50 FPS (GPU)
- **Accuracy**: Better
- **Use case**: Balance of speed and accuracy

### YOLOv8m
- **Size**: ~52 MB
- **Speed**: 5-10 FPS on 1080p (CPU), 20-35 FPS (GPU)
- **Accuracy**: Very good
- **Use case**: Accuracy-critical applications with GPU

### MobileNet SSD
- **Size**: ~23 MB
- **Speed**: 20-30 FPS on 1080p (CPU)
- **Accuracy**: Moderate
- **Use case**: CPU-only deployments

## Installation

After downloading, place models in this directory:

```bash
sudo mkdir -p /opt/wave_analytics/models
sudo cp *.onnx /opt/wave_analytics/models/
sudo chmod 644 /opt/wave_analytics/models/*
```

Or copy to this directory for development:

```bash
cp *.onnx ./
```

## Configuration

Update the model path in your configuration file:

```json
{
  "detection": {
    "model": "yolov8n",
    "modelPath": "/opt/wave_analytics/models/yolov8n.onnx"
  }
}
```

## Performance Guidelines

### CPU-Only Systems
- Use **YOLOv8n** or **MobileNet SSD**
- Set `frameSkipRate: 2` or higher
- Limit to 2-4 cameras per server

### GPU Systems (CUDA)
- Use **YOLOv8s** or **YOLOv8m**
- Can process 6-12 cameras per GPU
- Enable GPU in config: `"useGPU": true`

### Resolution Recommendations

**1080p (1920x1080)**:
- YOLOv8n: 15-25 FPS (CPU), 45-80 FPS (GPU)
- YOLOv8s: 10-15 FPS (CPU), 30-50 FPS (GPU)

**4K (3840x2160)**:
- YOLOv8n with frameSkip=3: 5-8 FPS (CPU), 15-25 FPS (GPU)
- Consider processing at 1080p instead

## License

Models are provided by their respective creators:
- **YOLOv8**: Ultralytics (AGPL-3.0)
- **MobileNet SSD**: Google (Apache 2.0)

Ensure compliance with model licenses for your use case.

## Troubleshooting

If model fails to load:

1. **Verify format**:
   ```bash
   file yolov8n.onnx
   # Should show: "data"
   ```

2. **Check size**:
   ```bash
   ls -lh yolov8n.onnx
   # Should be ~6 MB for yolov8n
   ```

3. **Test with OpenCV**:
   ```python
   import cv2
   net = cv2.dnn.readNetFromONNX('yolov8n.onnx')
   print("Model loaded successfully")
   ```

4. **Re-download if corrupted**:
   ```bash
   rm yolov8n.onnx
   wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.onnx
   ```
