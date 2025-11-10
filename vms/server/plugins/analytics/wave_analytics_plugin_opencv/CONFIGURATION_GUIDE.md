# Wave Analytics Plugin - Configuration Guide

This guide explains all configuration options available for tweaking the plugin's behavior.

## 📂 Configuration File Locations

The plugin searches for configuration files in this order:

1. **Device-specific**: `/etc/wave_analytics/device_{DEVICE_ID}.json`
2. **System-wide**: `/etc/wave_analytics/config.json`
3. **Installation**: `/opt/wave_analytics/config.json`
4. **Local (development)**: `./config/config.json`
5. **Relative (development)**: `../config/config.json`

**First valid configuration file found is used.** If none found, default settings are used.

---

## 🔧 Complete Configuration Reference

### Detection Settings

```json
{
  "detection": {
    "model": "yolov8n",
    "modelPath": "/opt/wave_analytics/models/yolov8n.onnx",
    "confidence": 0.5,
    "nmsThreshold": 0.4,
    "useGPU": true
  }
}
```

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `model` | string | `"yolov8n"` | See below | AI model to use for detection |
| `modelPath` | string | `/opt/wave_analytics/models/yolov8n.onnx` | - | Full path to model file |
| `confidence` | float | `0.5` | 0.1 - 1.0 | Minimum confidence threshold for detections |
| `nmsThreshold` | float | `0.4` | 0.1 - 1.0 | Non-maximum suppression threshold |
| `useGPU` | boolean | `true` | true/false | Enable CUDA GPU acceleration |

**Supported Models:**
- `yolov8n` - Fastest, ~6 MB, 15-25 FPS (CPU), 45-80 FPS (GPU)
- `yolov8s` - Balanced, ~22 MB, 10-15 FPS (CPU), 30-50 FPS (GPU)
- `yolov8m` - Better accuracy, ~52 MB, 5-10 FPS (CPU), 20-35 FPS (GPU)
- `yolov8l` - High accuracy, requires GPU
- `yolov8x` - Best accuracy, requires powerful GPU
- `mobilenet_ssd` - Fast CPU inference, moderate accuracy

---

### Tracking Settings

```json
{
  "tracking": {
    "maxAge": 30,
    "minHits": 3,
    "iouThreshold": 0.3
  }
}
```

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `maxAge` | int | `30` | 10 - 100 | Max frames to keep track without detection |
| `minHits` | int | `3` | 1 - 10 | Min detections before track confirmed |
| `iouThreshold` | float | `0.3` | 0.1 - 0.9 | IOU threshold for track association |

**Tuning Tips:**
- **Increase maxAge** (e.g., 50) if objects are frequently occluded
- **Decrease minHits** (e.g., 1) for immediate tracking
- **Lower iouThreshold** (e.g., 0.2) for faster moving objects

---

### Analytics Features

```json
{
  "analytics": {
    "loiteringTimeSeconds": 30,
    "enableFallDetection": true,
    "enableAggressiveBehavior": true,
    "enableQueueAnalytics": true,
    "enableCrowdDensity": true
  }
}
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `loiteringTimeSeconds` | int | `30` | Time threshold for loitering detection (seconds) |
| `enableFallDetection` | boolean | `true` | Enable/disable fall detection for persons |
| `enableAggressiveBehavior` | boolean | `true` | Enable/disable aggressive behavior detection |
| `enableQueueAnalytics` | boolean | `true` | Enable/disable queue management analytics |
| `enableCrowdDensity` | boolean | `true` | Enable/disable crowd density analysis |

**Feature-specific Settings:**

- **Loitering**: Can be overridden per zone (see Zones section)
- **Fall Detection**: Uses aspect ratio and height changes
- **Aggressive Behavior**: Detects rapid acceleration (>15 pixels/frame²)
- **Queue Analytics**: Requires properly configured zones
- **Crowd Density**: See crowd density thresholds below

---

### Crowd Density Thresholds

```json
{
  "crowdDensity": {
    "low": 5,
    "medium": 10,
    "high": 20,
    "critical": 30
  }
}
```

| Level | Default | Description |
|-------|---------|-------------|
| `low` | 5 | Transition from normal to low density |
| `medium` | 10 | Transition from low to medium density |
| `high` | 20 | Transition from medium to high density |
| `critical` | 30 | Transition from high to critical density |

**Example Settings by Use Case:**
- **Small Room**: `{2, 5, 8, 12}`
- **Retail Store Aisle**: `{5, 10, 20, 30}` (default)
- **Large Hall**: `{20, 40, 60, 100}`
- **Stadium Section**: `{50, 100, 200, 300}`

---

### Performance Settings

```json
{
  "performance": {
    "frameSkipRate": 1,
    "maxObjectsPerFrame": 100,
    "enableVisualization": true
  }
}
```

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `frameSkipRate` | int | `1` | 1 - 10 | Process every Nth frame (1 = every frame) |
| `maxObjectsPerFrame` | int | `100` | 10 - 500 | Maximum objects to track simultaneously |
| `enableVisualization` | boolean | `true` | true/false | Draw bounding boxes on video |

**Performance Tuning:**

**High Load (many cameras):**
```json
{
  "frameSkipRate": 3,
  "maxObjectsPerFrame": 50,
  "enableVisualization": false
}
```

**High Accuracy (few cameras):**
```json
{
  "frameSkipRate": 1,
  "maxObjectsPerFrame": 200,
  "enableVisualization": true
}
```

---

### Zones Configuration

Zones define areas in the video for specific analytics. All coordinates are in **pixels**.

```json
{
  "zones": [
    {
      "name": "entrance",
      "type": "queue_entrance",
      "coordinates": [[100, 100], [300, 100], [300, 200], [100, 200]],
      "loiteringThresholdSeconds": 15,
      "maxOccupancy": 10,
      "activeAtNight": true,
      "startHour": 18,
      "endHour": 6
    }
  ]
}
```

#### Zone Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Unique zone identifier |
| `type` | string | ✅ | Zone type (see below) |
| `coordinates` | array | ✅ | Polygon vertices [[x1,y1], [x2,y2], ...] |
| `loiteringThresholdSeconds` | int | ❌ | Override global loitering threshold |
| `maxOccupancy` | int | ❌ | Maximum allowed occupancy (-1 = unlimited) |
| `activeAtNight` | boolean | ❌ | Only active during night hours |
| `startHour` | int | ❌ | Start hour for time-based activation (0-23) |
| `endHour` | int | ❌ | End hour for time-based activation (0-23) |

#### Zone Types

| Type | Description | Used For |
|------|-------------|----------|
| `queue_entrance` | Entry point | Queue analytics (customer enters) |
| `queue_waiting` | Waiting area | Queue analytics (customer waiting) |
| `queue_service` | Service counter | Queue analytics (customer being served) |
| `queue_exit` | Exit area | Queue analytics (customer exits) |
| `restricted` | Restricted access | Intrusion detection |
| `intrusion` | High-security area | Intrusion detection (high priority) |
| `monitoring` | General monitoring | Loitering, crowd density |

#### Zone Coordinate System

For 1920x1080 video:
```
(0,0) ──────────────────── (1920,0)
  │                           │
  │     Your Video Frame     │
  │                           │
(0,1080) ────────────────── (1920,1080)
```

**Example Zones:**

**Entire Frame:**
```json
[[0, 0], [1920, 0], [1920, 1080], [0, 1080]]
```

**Bottom Half:**
```json
[[0, 540], [1920, 540], [1920, 1080], [0, 1080]]
```

**Centered Rectangle:**
```json
[[480, 270], [1440, 270], [1440, 810], [480, 810]]
```

---

### Trip Lines Configuration

Trip lines (virtual tripwires) detect when objects cross a line.

```json
{
  "tripLines": [
    {
      "name": "entrance_counter",
      "start": [100, 500],
      "end": [900, 500],
      "bidirectional": true,
      "directionAtoB": "entering",
      "directionBtoA": "exiting"
    }
  ]
}
```

#### Trip Line Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Unique line identifier |
| `start` | array | ✅ | Start point [x, y] |
| `end` | array | ✅ | End point [x, y] |
| `bidirectional` | boolean | ✅ | Count both directions |
| `directionAtoB` | string | ✅ | Label for start→end crossing |
| `directionBtoA` | string | ❌ | Label for end→start crossing |

**Example Use Cases:**

**Door Counter (bidirectional):**
```json
{
  "name": "main_door",
  "start": [960, 200],
  "end": [960, 880],
  "bidirectional": true,
  "directionAtoB": "person_entered",
  "directionBtoA": "person_exited"
}
```

**Exit Gate (one-way):**
```json
{
  "name": "exit_gate",
  "start": [100, 600],
  "end": [1820, 600],
  "bidirectional": false,
  "directionAtoB": "vehicle_exited"
}
```

**Perimeter Fence:**
```json
{
  "name": "perimeter_breach",
  "start": [0, 400],
  "end": [1920, 400],
  "bidirectional": false,
  "directionAtoB": "fence_breach_detected"
}
```

---

### Business Rules Configuration

Create custom rules with flexible conditions.

```json
{
  "rules": [
    {
      "name": "No vehicles after hours",
      "condition": "(object.type == 'car' || object.type == 'truck') && (time.hour >= 20 || time.hour < 6)",
      "action": "alert",
      "priority": "high",
      "enabled": true
    }
  ]
}
```

#### Rule Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `name` | string | ✅ | Unique rule identifier |
| `condition` | string | ❌* | Boolean expression (see below) |
| `objectType` | string | ❌* | Filter by object type |
| `zone` | string | ❌ | Filter by zone name |
| `minCount` | int | ❌ | Minimum object count in zone |
| `maxCount` | int | ❌ | Maximum object count in zone |
| `minSpeed` | int | ❌ | Minimum speed threshold |
| `action` | string | ✅ | Action to take (usually "alert") |
| `priority` | string | ✅ | Priority: "low", "medium", "high" |
| `startHour` | int | ❌ | Start hour for rule (0-23) |
| `endHour` | int | ❌ | End hour for rule (0-23) |
| `enabled` | boolean | ✅ | Enable/disable rule |

*Either `condition` or `objectType` is required

#### Condition Expression Syntax

**Variables:**
- `object.type` - Object class ("person", "car", "truck", etc.)
- `object.speed` - Object speed in pixels/frame
- `object.trackId` - Unique track ID
- `time.hour` - Current hour (0-23)
- `zone.{zoneName}` - Boolean, true if object in zone
- `zone.count` - Object count in current zone

**Operators:**
- Comparison: `==`, `!=`, `>`, `<`, `>=`, `<=`
- Logic: `&&` (AND), `||` (OR)
- Grouping: `( )`

**Examples:**

**Simple object type filter:**
```json
{
  "condition": "object.type == 'person'"
}
```

**Time-based:**
```json
{
  "condition": "time.hour >= 22 || time.hour < 6"
}
```

**Multiple conditions (AND):**
```json
{
  "condition": "object.type == 'truck' && object.speed > 20"
}
```

**Multiple conditions (OR):**
```json
{
  "condition": "object.type == 'car' || object.type == 'truck' || object.type == 'bus'"
}
```

**Zone-based:**
```json
{
  "condition": "object.type == 'person' && zone.restricted_area == true"
}
```

**Complex (with grouping):**
```json
{
  "condition": "(object.type == 'person' && time.hour >= 18) || (object.type == 'vehicle' && zone.parking == true)"
}
```

#### Simple Rules (without expressions)

For simple cases, use parameter-based rules:

```json
{
  "name": "Too many people in lobby",
  "objectType": "person",
  "zone": "lobby",
  "minCount": 15,
  "priority": "medium",
  "enabled": true
}
```

```json
{
  "name": "Fast moving vehicle",
  "objectType": "car",
  "minSpeed": 30,
  "priority": "high",
  "enabled": true
}
```

---

## 📋 Configuration Templates

### Template 1: Retail Store

Optimized for checkout queue management.

```json
{
  "detection": {
    "model": "yolov8n",
    "confidence": 0.6,
    "useGPU": true
  },
  "analytics": {
    "loiteringTimeSeconds": 20,
    "enableQueueAnalytics": true,
    "enableCrowdDensity": true,
    "enableFallDetection": false,
    "enableAggressiveBehavior": false
  },
  "crowdDensity": {
    "low": 5,
    "medium": 10,
    "high": 20,
    "critical": 30
  },
  "zones": [
    {
      "name": "store_entrance",
      "type": "queue_entrance",
      "coordinates": [[0, 400], [200, 400], [200, 700], [0, 700]]
    },
    {
      "name": "checkout_queue_1",
      "type": "queue_waiting",
      "coordinates": [[200, 100], [500, 100], [500, 350], [200, 350]],
      "maxOccupancy": 12
    },
    {
      "name": "checkout_counter_1",
      "type": "queue_service",
      "coordinates": [[500, 100], [650, 100], [650, 350], [500, 350]]
    }
  ],
  "rules": [
    {
      "name": "Long queue alert",
      "zone": "checkout_queue_1",
      "minCount": 10,
      "priority": "high",
      "enabled": true
    }
  ]
}
```

### Template 2: Security Monitoring

Optimized for perimeter security and intrusion detection.

```json
{
  "detection": {
    "model": "yolov8s",
    "confidence": 0.4,
    "useGPU": true
  },
  "analytics": {
    "loiteringTimeSeconds": 60,
    "enableFallDetection": true,
    "enableAggressiveBehavior": true,
    "enableQueueAnalytics": false,
    "enableCrowdDensity": false
  },
  "performance": {
    "frameSkipRate": 2
  },
  "zones": [
    {
      "name": "perimeter",
      "type": "restricted",
      "coordinates": [[50, 50], [1870, 50], [1870, 1030], [50, 1030]],
      "activeAtNight": true,
      "startHour": 20,
      "endHour": 7
    },
    {
      "name": "loading_dock",
      "type": "intrusion",
      "coordinates": [[100, 700], [600, 700], [600, 1000], [100, 1000]],
      "loiteringThresholdSeconds": 120
    }
  ],
  "tripLines": [
    {
      "name": "perimeter_fence",
      "start": [0, 500],
      "end": [1920, 500],
      "bidirectional": false,
      "directionAtoB": "perimeter_breach"
    }
  ],
  "rules": [
    {
      "name": "Person in restricted area at night",
      "condition": "object.type == 'person' && (time.hour >= 20 || time.hour < 7)",
      "zone": "perimeter",
      "priority": "critical",
      "enabled": true
    },
    {
      "name": "Vehicle near loading dock",
      "condition": "object.type == 'car' || object.type == 'truck'",
      "zone": "loading_dock",
      "priority": "high",
      "enabled": true
    }
  ]
}
```

### Template 3: Traffic Monitoring

Vehicle and pedestrian counting.

```json
{
  "detection": {
    "model": "yolov8s",
    "confidence": 0.5,
    "useGPU": true
  },
  "analytics": {
    "loiteringTimeSeconds": 60,
    "enableQueueAnalytics": false,
    "enableCrowdDensity": false
  },
  "tripLines": [
    {
      "name": "northbound_lane",
      "start": [600, 0],
      "end": [600, 1080],
      "bidirectional": false,
      "directionAtoB": "vehicle_northbound"
    },
    {
      "name": "southbound_lane",
      "start": [1200, 1080],
      "end": [1200, 0],
      "bidirectional": false,
      "directionAtoB": "vehicle_southbound"
    },
    {
      "name": "crosswalk",
      "start": [0, 540],
      "end": [1920, 540],
      "bidirectional": true,
      "directionAtoB": "pedestrian_crossing"
    }
  ]
}
```

---

## 🛠️ Configuration Tools

### Validate Configuration

```bash
# Test load configuration
python3 -c "import json; json.load(open('/etc/wave_analytics/config.json'))"
```

### Apply Configuration

1. Edit configuration file:
   ```bash
   sudo nano /etc/wave_analytics/config.json
   ```

2. Restart Wave Server to reload:
   ```bash
   sudo systemctl restart networkoptix-mediaserver
   ```

3. Check logs for confirmation:
   ```bash
   sudo journalctl -u networkoptix-mediaserver | grep "Configuration loaded"
   ```

---

## 🔍 Troubleshooting Configuration

### Configuration Not Loading

**Check file exists:**
```bash
ls -l /etc/wave_analytics/config.json
```

**Check JSON syntax:**
```bash
python3 -m json.tool /etc/wave_analytics/config.json
```

**Check logs:**
```bash
sudo journalctl -u networkoptix-mediaserver -f | grep -i config
```

### Model File Not Found

Update model path in config:
```json
{
  "detection": {
    "modelPath": "/path/to/your/model.onnx"
  }
}
```

### Too Many/Few Detections

**Too many (false positives):**
- Increase `confidence`: 0.5 → 0.7
- Increase `nmsThreshold`: 0.4 → 0.6

**Too few (missing objects):**
- Decrease `confidence`: 0.5 → 0.3
- Decrease `nmsThreshold`: 0.4 → 0.3

---

## 📚 Additional Resources

- **README.md** - Installation and build instructions
- **EXAMPLES.md** - Real-world configuration examples
- **TROUBLESHOOTING.md** - Common issues and solutions

For more help, check the plugin logs:
```bash
sudo journalctl -u networkoptix-mediaserver -f | grep wave_opencv
```
