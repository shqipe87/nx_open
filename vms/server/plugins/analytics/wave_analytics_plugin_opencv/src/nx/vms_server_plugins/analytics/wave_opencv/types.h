// Copyright 2024. All Rights Reserved.
//
// Common types and structures for Wave Analytics Plugin

#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

// Object detection result
struct Detection
{
    cv::Rect bbox;                  // Bounding box
    int classId;                    // Class ID (person, vehicle, etc.)
    std::string className;          // Human-readable class name
    float confidence;               // Detection confidence [0-1]
    cv::Point2f center;            // Center point
};

// Tracked object across frames
struct TrackedObject
{
    int trackId;                    // Unique track ID
    Detection detection;            // Current detection
    std::vector<cv::Point2f> trajectory;  // Historical positions
    int64_t firstSeenTimestampUs;  // First appearance timestamp
    int64_t lastSeenTimestampUs;   // Last seen timestamp
    int framesSinceLastSeen;        // Frames without detection
    bool isActive;                  // Currently being tracked

    // Motion features
    cv::Point2f velocity;           // Current velocity (pixels/frame)
    cv::Point2f acceleration;       // Current acceleration
    float speed;                    // Speed magnitude
    float direction;                // Movement direction (radians)

    // State features
    int stationaryFrames;           // Frames with minimal movement
    bool isStationary;              // Currently stationary
    float aspectRatio;              // Width/height ratio
    float heightHistory[10];        // Recent height values for fall detection
    int heightHistoryIndex;

    // Zone tracking
    std::vector<std::string> currentZones;  // Zones object is currently in
    std::map<std::string, int64_t> zoneEntryTimes;  // When entered each zone
};

// Zone definition
struct Zone
{
    std::string name;               // Zone name
    std::string type;               // queue_entrance, restricted, etc.
    std::vector<cv::Point> polygon; // Zone boundary
    cv::Rect boundingBox;          // Bounding box for quick checks

    // Zone-specific parameters
    int loiteringThresholdSeconds = 30;
    int maxOccupancy = -1;          // -1 = unlimited
    bool activeAtNight = true;
    int startHour = 0;
    int endHour = 24;
};

// Line crossing definition
struct TripLine
{
    std::string name;               // Line name
    cv::Point start;                // Start point
    cv::Point end;                  // End point
    bool bidirectional;             // Count both directions
    std::string directionAtoB;      // Name for A→B crossing
    std::string directionBtoA;      // Name for B→A crossing
};

// Analytics event
struct AnalyticsEvent
{
    std::string eventType;          // Event type ID
    std::string caption;            // Event title
    std::string description;        // Detailed description
    int64_t timestampUs;           // Event timestamp
    int64_t durationUs;            // Event duration (0 for momentary)
    cv::Rect boundingBox;          // Associated bounding box
    int trackId;                    // Associated track ID
    std::string severity;           // low, medium, high, critical
    std::map<std::string, std::string> attributes;  // Custom attributes
};

// Queue metrics
struct QueueMetrics
{
    int currentQueueLength;
    float averageWaitTimeSeconds;
    float averageServiceTimeSeconds;
    int customersServedLastHour;
    int abandonedLastHour;
    float abandonmentRate;
    std::vector<int> hourlyPeaks;   // Queue length per hour
};

// Business rule
struct BusinessRule
{
    std::string name;               // Rule name
    std::string condition;          // Condition expression
    std::string action;             // Action to take
    std::string priority;           // low, medium, high
    bool enabled;                   // Rule enabled/disabled

    // Condition parameters (for simple rules)
    std::string objectType;         // Filter by object type
    std::string zone;               // Filter by zone
    int minCount = -1;              // Minimum object count
    int maxCount = -1;              // Maximum object count
    int minSpeed = -1;              // Minimum speed threshold
    int startHour = 0;              // Time range start
    int endHour = 24;               // Time range end
};

// Configuration structure
struct PluginConfiguration
{
    // Detection settings
    std::string modelType;          // yolov8n, yolov8s, mobilenet_ssd
    std::string modelPath;          // Path to model file
    float confidenceThreshold;      // Detection confidence threshold
    float nmsThreshold;             // Non-maximum suppression threshold
    bool useGPU;                    // Enable CUDA acceleration

    // Tracking settings
    int maxAge;                     // Max frames to keep track without detection
    int minHits;                    // Min detections before track confirmed
    float iouThreshold;             // IOU threshold for matching

    // Analytics settings
    int loiteringTimeSeconds;       // Loitering threshold
    bool enableFallDetection;       // Enable fall detection
    bool enableAggressiveBehavior;  // Enable aggressive behavior detection
    bool enableQueueAnalytics;      // Enable queue management
    bool enableCrowdDensity;        // Enable crowd density analysis

    // Crowd density thresholds
    int crowdLowThreshold;
    int crowdMediumThreshold;
    int crowdHighThreshold;
    int crowdCriticalThreshold;

    // Performance settings
    int frameSkipRate;              // Process every Nth frame
    int maxObjectsPerFrame;         // Max objects to track
    bool enableVisualization;       // Draw bounding boxes

    // Zones and rules
    std::vector<Zone> zones;
    std::vector<TripLine> tripLines;
    std::vector<BusinessRule> rules;

    // Default constructor with sensible defaults
    PluginConfiguration()
        : modelType("yolov8n")
        , modelPath("/opt/wave_analytics/models/yolov8n.onnx")
        , confidenceThreshold(0.5f)
        , nmsThreshold(0.4f)
        , useGPU(true)
        , maxAge(30)
        , minHits(3)
        , iouThreshold(0.3f)
        , loiteringTimeSeconds(30)
        , enableFallDetection(true)
        , enableAggressiveBehavior(true)
        , enableQueueAnalytics(true)
        , enableCrowdDensity(true)
        , crowdLowThreshold(5)
        , crowdMediumThreshold(10)
        , crowdHighThreshold(20)
        , crowdCriticalThreshold(30)
        , frameSkipRate(1)
        , maxObjectsPerFrame(100)
        , enableVisualization(true)
    {
    }
};

// COCO class names (80 classes)
static const std::vector<std::string> COCO_CLASSES = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
    "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
    "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
    "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
    "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
    "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
    "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator",
    "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
};

// Helper functions
inline bool isPointInPolygon(const cv::Point2f& point, const std::vector<cv::Point>& polygon)
{
    return cv::pointPolygonTest(polygon, point, false) >= 0;
}

inline float calculateIOU(const cv::Rect& box1, const cv::Rect& box2)
{
    cv::Rect intersection = box1 & box2;
    float intersectionArea = intersection.area();
    float unionArea = box1.area() + box2.area() - intersectionArea;
    return unionArea > 0 ? intersectionArea / unionArea : 0.0f;
}

inline float calculateDistance(const cv::Point2f& p1, const cv::Point2f& p2)
{
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline int lineOrientation(const cv::Point& p, const cv::Point& q, const cv::Point& r)
{
    int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (val == 0) return 0;
    return (val > 0) ? 1 : 2;
}

inline bool doSegmentsIntersect(const cv::Point& p1, const cv::Point& q1,
                                 const cv::Point& p2, const cv::Point& q2)
{
    int o1 = lineOrientation(p1, q1, p2);
    int o2 = lineOrientation(p1, q1, q2);
    int o3 = lineOrientation(p2, q2, p1);
    int o4 = lineOrientation(p2, q2, q1);

    if (o1 != o2 && o3 != o4) return true;
    return false;
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
