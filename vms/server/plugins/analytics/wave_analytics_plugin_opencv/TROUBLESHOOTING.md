# Troubleshooting Guide

## Plugin Issues

### Plugin Not Appearing in Wave Client

**Symptoms:**
- Plugin not listed in Settings → Plugins

**Solutions:**
1. Verify plugin file exists:
   ```bash
   ls -l /opt/networkoptix/mediaserver/bin/plugins/wave_analytics_plugin_opencv.so
   ```

2. Check file permissions:
   ```bash
   sudo chmod 755 /opt/networkoptix/mediaserver/bin/plugins/wave_analytics_plugin_opencv.so
   ```

3. Verify dependencies:
   ```bash
   ldd /opt/networkoptix/mediaserver/bin/plugins/wave_analytics_plugin_opencv.so
   # Should not show "not found" for any libraries
   ```

4. Check server logs:
   ```bash
   sudo journalctl -u networkoptix-mediaserver -f | grep wave_opencv
   ```

5. Restart Wave Server:
   ```bash
   sudo systemctl restart networkoptix-mediaserver
   ```

---

### Plugin Crashes on Load

**Symptoms:**
- Server crashes when enabling plugin
- Error in logs: "Segmentation fault"

**Solutions:**
1. Check OpenCV version compatibility:
   ```bash
   pkg-config --modversion opencv4
   # Should be 4.5.0 or higher
   ```

2. Verify model file exists and is readable:
   ```bash
   ls -l /opt/wave_analytics/models/yolov8n.onnx
   ```

3. Check for missing CUDA libraries (if GPU enabled):
   ```bash
   nvidia-smi  # Should show GPU info
   ldconfig -p | grep cuda  # Should list CUDA libraries
   ```

4. Rebuild plugin with debug symbols:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

5. Run server in foreground to see crash details:
   ```bash
   sudo systemctl stop networkoptix-mediaserver
   sudo /opt/networkoptix/mediaserver/bin/mediaserver -d
   ```

---

## Detection Issues

### No Objects Detected

**Symptoms:**
- Plugin runs but no objects appear
- No bounding boxes visible

**Solutions:**
1. Lower confidence threshold:
   - Try 0.3 instead of 0.5
   - Edit in plugin settings or config.json

2. Verify model file is correct format:
   ```bash
   file /opt/wave_analytics/models/yolov8n.onnx
   # Should show: "data"
   ```

3. Check video feed resolution:
   - Plugin works best with 720p or 1080p
   - Very low resolution may not detect well

4. Improve lighting:
   - Ensure adequate illumination
   - Avoid extreme backlighting

5. Test with known good video:
   - Use sample video with clear objects
   - If works, issue is with camera feed

---

### Poor Detection Accuracy

**Symptoms:**
- Many false positives/negatives
- Objects frequently lost

**Solutions:**
1. Increase confidence threshold:
   ```json
   "confidence": 0.6  // Instead of 0.4
   ```

2. Use better model:
   - Switch from yolov8n to yolov8s
   - Download: `wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8s.onnx`

3. Adjust NMS threshold:
   ```json
   "nmsThreshold": 0.5  // Instead of 0.4
   ```

4. Optimize camera settings:
   - Increase camera resolution
   - Adjust camera angle for better view
   - Enable WDR (Wide Dynamic Range)

---

## Performance Issues

### High CPU Usage

**Symptoms:**
- CPU at 100% constantly
- Server becomes unresponsive

**Solutions:**
1. Increase frame skip rate:
   ```json
   "frameSkipRate": 3  // Process every 3rd frame
   ```

2. Use faster model:
   - Switch to yolov8n (fastest)
   - Or mobilenet_ssd

3. Disable unnecessary analytics:
   ```json
   "enableQueueAnalytics": false,
   "enableCrowdDensity": false
   ```

4. Reduce number of cameras:
   - Don't enable plugin on all cameras
   - Use on critical cameras only

5. Lower video resolution:
   - Process at 720p instead of 1080p
   - Configure in camera settings

---

### High GPU Memory Usage

**Symptoms:**
- GPU memory full
- CUDA out of memory errors

**Solutions:**
1. Disable GPU acceleration:
   ```json
   "useGPU": false
   ```

2. Process fewer streams simultaneously:
   - Limit to 4-6 cameras per GPU

3. Use smaller model:
   - yolov8n uses less GPU memory than yolov8s/m

4. Reduce batch size (requires code change):
   - Modify detector to process single images

---

### Slow Processing / Low FPS

**Symptoms:**
- Processing < 5 FPS
- Significant lag in detections

**Solutions:**
1. Enable GPU acceleration:
   ```json
   "useGPU": true
   ```

2. Verify CUDA is being used:
   ```bash
   # Check logs for "Using CUDA backend"
   sudo journalctl -u networkoptix-mediaserver | grep -i cuda
   ```

3. Use faster model:
   ```json
   "model": "yolov8n"  // Fastest
   ```

4. Optimize OpenCV build:
   ```bash
   # Rebuild OpenCV with optimizations
   cmake -DWITH_CUDA=ON \
         -DCUDA_ARCH_BIN=7.5 \
         -DENABLE_FAST_MATH=ON \
         -DCMAKE_BUILD_TYPE=Release
   ```

---

## Event Issues

### Events Not Appearing

**Symptoms:**
- Objects detected but no events generated
- Events panel empty

**Solutions:**
1. Verify event types are enabled in Wave Client:
   - Open camera settings
   - Check Analytics tab
   - Ensure event types are checked

2. Check zone configuration:
   - Zones may be outside camera view
   - Verify coordinates are correct

3. Review rule conditions:
   - Rules may be too restrictive
   - Test with simple rule first

4. Enable debug logging:
   - Look for "Event: " messages in logs
   - Verify events are being generated

---

### Too Many False Alarms

**Symptoms:**
- Constant loitering alerts
- Intrusion events from shadows/trees

**Solutions:**
1. Increase detection thresholds:
   ```json
   "loiteringTimeSeconds": 60,  // Instead of 30
   "confidenceThreshold": 0.6   // Instead of 0.4
   ```

2. Adjust zone boundaries:
   - Exclude problem areas
   - Make zones more specific

3. Add time constraints to rules:
   ```json
   {
     "startHour": 18,
     "endHour": 8
   }
   ```

4. Filter by object type:
   ```json
   "objectType": "person"  // Ignore animals, etc.
   ```

---

## Queue Analytics Issues

### Queue Metrics Incorrect

**Symptoms:**
- Queue length always 0
- Wait times not calculated

**Solutions:**
1. Verify zone types are correct:
   ```json
   "type": "queue_entrance"  // Not "restricted"
   ```

2. Check zone order:
   - Must have: entrance → waiting → service → exit
   - Objects must pass through in order

3. Ensure zones don't overlap excessively:
   - Small overlaps OK
   - Large overlaps cause confusion

4. Verify object tracking is working:
   - Check for consistent track IDs
   - Increase tracking parameters if needed

---

## Build Issues

### OpenCV Not Found

**Error:**
```
CMake Error: Could not find OpenCV
```

**Solutions:**
1. Install OpenCV development packages:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libopencv-dev

   # RHEL/CentOS
   sudo yum install opencv-devel
   ```

2. Specify OpenCV path:
   ```bash
   cmake -DOpenCV_DIR=/usr/local/lib/cmake/opencv4
   ```

3. Build OpenCV from source:
   ```bash
   git clone https://github.com/opencv/opencv.git
   cd opencv
   cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
   cmake --build build -j$(nproc)
   sudo cmake --install build
   ```

---

### Wave SDK Not Found

**Error:**
```
CMake Error: NX_SDK_DIR not found
```

**Solutions:**
1. Build within Wave repository:
   ```bash
   cd /path/to/nx_open
   cmake -B build
   ```

2. Or specify SDK path:
   ```bash
   cmake -DNX_SDK_DIR=/path/to/nx_open/vms/libs/nx_sdk
   ```

---

## Model Issues

### Model Download Failed

**Error:**
```
Failed to load YOLO model from: /opt/wave_analytics/models/yolov8n.onnx
```

**Solutions:**
1. Download model manually:
   ```bash
   sudo mkdir -p /opt/wave_analytics/models
   sudo wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.onnx \
     -O /opt/wave_analytics/models/yolov8n.onnx
   ```

2. Verify file integrity:
   ```bash
   file /opt/wave_analytics/models/yolov8n.onnx
   # Should show: "data"
   ```

3. Check file permissions:
   ```bash
   sudo chmod 644 /opt/wave_analytics/models/yolov8n.onnx
   ```

4. Update model path in config:
   ```json
   "modelPath": "/path/to/your/model.onnx"
   ```

---

## Getting Help

If issues persist:

1. **Enable debug logging:**
   ```bash
   sudo journalctl -u networkoptix-mediaserver -f
   ```

2. **Collect information:**
   - Wave Server version
   - OpenCV version
   - GPU model (if using CUDA)
   - OS version
   - Error messages from logs

3. **Check example configurations:**
   - See `EXAMPLES.md` for working configs
   - Start with simplest configuration

4. **Review SDK documentation:**
   - `/vms/libs/nx_sdk/src/nx/sdk/analytics/readme.md`
   - SDK examples in repository

5. **Test with sample video:**
   - Eliminate camera/network issues
   - Use known good test video

## Common Log Messages

**Normal:**
```
Wave Analytics Plugin with OpenCV initialized
Analytics pipeline initialized successfully
Object detector initialized successfully with model: yolov8n
Using CUDA backend for inference
```

**Warning:**
```
Using CPU backend for inference  // GPU not available/enabled
Failed to download YOLOv8n model  // Download model manually
```

**Error:**
```
Failed to initialize object detector  // Check model path
Exception during detection: ...  // Model incompatible
Segmentation fault  // Missing dependency or bad build
```
