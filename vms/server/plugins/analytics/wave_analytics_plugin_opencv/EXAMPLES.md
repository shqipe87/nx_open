# Wave Analytics Plugin - Usage Examples

This document provides practical examples for common use cases.

## Example 1: Retail Store Queue Management

### Scenario
Monitor checkout queues in a retail store with 2 checkout lanes.

### Configuration
```json
{
  "detection": {
    "model": "yolov8n",
    "confidence": 0.6
  },
  "analytics": {
    "enableQueueAnalytics": true,
    "enableCrowdDensity": true
  },
  "zones": [
    {
      "name": "entrance",
      "type": "queue_entrance",
      "coordinates": [[0, 500], [200, 500], [200, 700], [0, 700]]
    },
    {
      "name": "queue_lane_1",
      "type": "queue_waiting",
      "coordinates": [[200, 100], [400, 100], [400, 350], [200, 350]],
      "maxOccupancy": 8
    },
    {
      "name": "checkout_1",
      "type": "queue_service",
      "coordinates": [[400, 100], [500, 100], [500, 350], [400, 350]]
    },
    {
      "name": "queue_lane_2",
      "type": "queue_waiting",
      "coordinates": [[200, 400], [400, 400], [400, 650], [200, 650]],
      "maxOccupancy": 8
    },
    {
      "name": "checkout_2",
      "type": "queue_service",
      "coordinates": [[400, 400], [500, 400], [500, 650], [400, 650]]
    }
  ]
}
```

### Expected Events
- **Queue Length Exceeded**: Alert when queue has > 8 people
- **Long Wait Time**: Alert when average wait > 5 minutes
- **Customer Abandoned Queue**: Track abandonment rate
- **Crowd Density**: Monitor overall store density

### Metrics Available
- Current queue length per lane
- Average wait time (last hour)
- Service rate (customers/hour)
- Abandonment rate
- Peak hours identification

---

## Example 2: Perimeter Security Monitoring

### Scenario
Monitor restricted areas and detect intrusions at a facility.

### Configuration
```json
{
  "detection": {
    "model": "yolov8s",
    "confidence": 0.4
  },
  "analytics": {
    "loiteringTimeSeconds": 60,
    "enableFallDetection": true,
    "enableAggressiveBehavior": true
  },
  "zones": [
    {
      "name": "building_perimeter",
      "type": "restricted",
      "coordinates": [[50, 50], [1870, 50], [1870, 1030], [50, 1030]],
      "activeAtNight": true,
      "startHour": 20,
      "endHour": 7
    },
    {
      "name": "loading_dock",
      "type": "intrusion",
      "coordinates": [[100, 800], [500, 800], [500, 1000], [100, 1000]],
      "loiteringThresholdSeconds": 120
    }
  ],
  "tripLines": [
    {
      "name": "fence_line",
      "start": [0, 600],
      "end": [1920, 600],
      "bidirectional": false,
      "directionAtoB": "perimeter_breach"
    }
  ],
  "rules": [
    {
      "name": "Person in restricted area at night",
      "condition": "object.type == 'person' && (time.hour >= 20 || time.hour < 7)",
      "zone": "building_perimeter",
      "priority": "critical",
      "enabled": true
    },
    {
      "name": "Vehicle near loading dock",
      "condition": "(object.type == 'car' || object.type == 'truck')",
      "zone": "loading_dock",
      "priority": "high",
      "enabled": true
    }
  ]
}
```

### Expected Events
- **Zone Intrusion**: Person enters restricted area
- **Loitering Detected**: Person stationary for > 60 seconds
- **Line Crossing**: Fence line breach
- **Business Rule Violation**: Custom rules triggered

---

## Example 3: ATM Monitoring

### Scenario
Monitor ATM area for loitering, aggressive behavior, and falls.

### Configuration
```json
{
  "detection": {
    "model": "yolov8n",
    "confidence": 0.5
  },
  "analytics": {
    "loiteringTimeSeconds": 120,
    "enableFallDetection": true,
    "enableAggressiveBehavior": true,
    "enableCrowdDensity": true
  },
  "zones": [
    {
      "name": "atm_area",
      "type": "monitoring",
      "coordinates": [[400, 200], [1000, 200], [1000, 800], [400, 800]],
      "loiteringThresholdSeconds": 120
    }
  ],
  "rules": [
    {
      "name": "Multiple people at ATM",
      "zone": "atm_area",
      "objectType": "person",
      "minCount": 3,
      "priority": "medium",
      "enabled": true
    },
    {
      "name": "Loitering at ATM after hours",
      "condition": "object.type == 'person' && (time.hour >= 22 || time.hour < 6)",
      "zone": "atm_area",
      "priority": "high",
      "enabled": true
    }
  ]
}
```

### Expected Events
- **Loitering Detected**: Person at ATM for > 2 minutes
- **Fall Detected**: Person falls in ATM area
- **Aggressive Behavior**: Rapid movements detected
- **Crowd Density**: Multiple people in small area

---

## Example 4: Parking Lot Monitoring

### Scenario
Monitor parking lot for unauthorized vehicles and pedestrian safety.

### Configuration
```json
{
  "detection": {
    "model": "yolov8s",
    "confidence": 0.5
  },
  "analytics": {
    "loiteringTimeSeconds": 300,
    "enableAggressiveBehavior": false
  },
  "zones": [
    {
      "name": "reserved_parking",
      "type": "restricted",
      "coordinates": [[100, 100], [500, 100], [500, 400], [100, 400]],
      "startHour": 8,
      "endHour": 18
    },
    {
      "name": "pedestrian_walkway",
      "type": "intrusion",
      "coordinates": [[600, 200], [800, 200], [800, 900], [600, 900]]
    }
  ],
  "tripLines": [
    {
      "name": "entrance_gate",
      "start": [50, 500],
      "end": [200, 500],
      "bidirectional": true,
      "directionAtoB": "vehicle_entered",
      "directionBtoA": "vehicle_exited"
    }
  ],
  "rules": [
    {
      "name": "Vehicle in pedestrian area",
      "condition": "(object.type == 'car' || object.type == 'truck' || object.type == 'motorcycle')",
      "zone": "pedestrian_walkway",
      "priority": "high",
      "enabled": true
    },
    {
      "name": "Unauthorized parking during work hours",
      "condition": "object.type == 'car' && time.hour >= 8 && time.hour < 18",
      "zone": "reserved_parking",
      "priority": "medium",
      "enabled": true
    }
  ]
}
```

### Expected Events
- **Line Crossing**: Vehicle entry/exit counts
- **Zone Intrusion**: Vehicle in pedestrian area
- **Loitering**: Vehicle parked for > 5 minutes
- **Business Rule Violation**: Unauthorized parking

---

## Example 5: Healthcare Facility - Fall Detection

### Scenario
Monitor patient areas for falls and loitering.

### Configuration
```json
{
  "detection": {
    "model": "yolov8n",
    "confidence": 0.6
  },
  "analytics": {
    "loiteringTimeSeconds": 45,
    "enableFallDetection": true,
    "enableCrowdDensity": false
  },
  "zones": [
    {
      "name": "patient_room_1",
      "type": "monitoring",
      "coordinates": [[0, 0], [640, 0], [640, 480], [0, 480]]
    },
    {
      "name": "hallway",
      "type": "monitoring",
      "coordinates": [[640, 0], [1920, 0], [1920, 300], [640, 300]],
      "loiteringThresholdSeconds": 60
    }
  ]
}
```

### Expected Events
- **Fall Detected**: Critical alert with location
- **Loitering Detected**: Patient stationary too long

---

## Example 6: Traffic Monitoring

### Scenario
Count vehicles crossing intersections and detect wrong-way traffic.

### Configuration
```json
{
  "detection": {
    "model": "yolov8s",
    "confidence": 0.5
  },
  "analytics": {
    "loiteringTimeSeconds": 60
  },
  "tripLines": [
    {
      "name": "northbound_lane",
      "start": [500, 0],
      "end": [500, 1080],
      "bidirectional": false,
      "directionAtoB": "northbound"
    },
    {
      "name": "southbound_lane",
      "start": [900, 1080],
      "end": [900, 0],
      "bidirectional": false,
      "directionAtoB": "southbound"
    },
    {
      "name": "crosswalk",
      "start": [0, 540],
      "end": [1920, 540],
      "bidirectional": true,
      "directionAtoB": "pedestrian_crossing"
    }
  ],
  "rules": [
    {
      "name": "Wrong way vehicle",
      "condition": "object.type == 'car' || object.type == 'truck'",
      "priority": "critical",
      "enabled": true
    }
  ]
}
```

### Expected Events
- **Line Crossing**: Vehicle counts per lane
- **Pedestrian Crossing**: Crosswalk usage

---

## Coordinate System

All coordinates are in pixels relative to video resolution:
- Origin (0, 0) is top-left corner
- X increases to the right
- Y increases downward

For 1920x1080 video:
- Top-left: [0, 0]
- Top-right: [1920, 0]
- Bottom-left: [0, 1080]
- Bottom-right: [1920, 1080]

## Tips

1. **Use correct zone types** for queue analytics to work properly
2. **Define zones in customer flow order**: entrance → waiting → service → exit
3. **Adjust confidence threshold** based on lighting and camera quality
4. **Start with simple rules** and add complexity as needed
5. **Use frame skip** on high-resolution cameras for better performance
6. **Test zone coordinates** with a single camera before deploying to all cameras

## Testing Configuration

Use Wave Client to:
1. Draw zones on video preview
2. Note pixel coordinates
3. Update JSON configuration
4. Reload plugin
5. Verify events in Events panel
