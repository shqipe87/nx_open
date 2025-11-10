// Copyright 2024. All Rights Reserved.

#include "integration.h"
#include "engine.h"

#include <nx/kit/debug.h>
#include <nx/kit/json.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

Integration::Integration()
{
    NX_PRINT << "Wave Analytics Plugin with OpenCV initialized";
}

Integration::~Integration()
{
}

std::string Integration::manifestString() const
{
    // Plugin-level manifest
    return /*suppress newline*/ 1 + (const char*)
R"json(
{
    "id": "nx.vms_server_plugins.analytics.wave_opencv",
    "name": "Wave Analytics Plugin with OpenCV",
    "description": "Advanced video analytics using OpenCV: object detection, tracking, behavioral analytics, queue management, and business rules",
    "version": "1.0.0",
    "vendor": "Wave Analytics",
    "engineSettingsModel": {
        "type": "Settings",
        "items": [
            {
                "type": "GroupBox",
                "caption": "Detection Settings",
                "items": [
                    {
                        "type": "ComboBox",
                        "name": "modelType",
                        "caption": "Detection Model",
                        "defaultValue": "yolov8n",
                        "range": [
                            "yolov8n",
                            "yolov8s",
                            "yolov8m",
                            "mobilenet_ssd"
                        ]
                    },
                    {
                        "type": "TextField",
                        "name": "modelPath",
                        "caption": "Model Path",
                        "defaultValue": "/opt/wave_analytics/models/yolov8n.onnx"
                    },
                    {
                        "type": "SpinBox",
                        "name": "confidenceThreshold",
                        "caption": "Confidence Threshold",
                        "defaultValue": 0.5,
                        "minValue": 0.1,
                        "maxValue": 1.0,
                        "decimals": 2
                    },
                    {
                        "type": "CheckBox",
                        "name": "useGPU",
                        "caption": "Use GPU Acceleration",
                        "defaultValue": true
                    }
                ]
            },
            {
                "type": "GroupBox",
                "caption": "Analytics Features",
                "items": [
                    {
                        "type": "CheckBox",
                        "name": "enableFallDetection",
                        "caption": "Enable Fall Detection",
                        "defaultValue": true
                    },
                    {
                        "type": "CheckBox",
                        "name": "enableAggressiveBehavior",
                        "caption": "Enable Aggressive Behavior Detection",
                        "defaultValue": true
                    },
                    {
                        "type": "CheckBox",
                        "name": "enableQueueAnalytics",
                        "caption": "Enable Queue Analytics",
                        "defaultValue": true
                    },
                    {
                        "type": "CheckBox",
                        "name": "enableCrowdDensity",
                        "caption": "Enable Crowd Density Analysis",
                        "defaultValue": true
                    },
                    {
                        "type": "SpinBox",
                        "name": "loiteringTimeSeconds",
                        "caption": "Loitering Threshold (seconds)",
                        "defaultValue": 30,
                        "minValue": 5,
                        "maxValue": 300
                    }
                ]
            },
            {
                        "type": "GroupBox",
                "caption": "Performance Settings",
                "items": [
                    {
                        "type": "SpinBox",
                        "name": "frameSkipRate",
                        "caption": "Process Every Nth Frame",
                        "defaultValue": 1,
                        "minValue": 1,
                        "maxValue": 10
                    },
                    {
                        "type": "CheckBox",
                        "name": "enableVisualization",
                        "caption": "Draw Bounding Boxes",
                        "defaultValue": true
                    }
                ]
            }
        ]
    }
}
)json";
}

Result<IEngine*> Integration::doObtainEngine()
{
    return new Engine();
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
