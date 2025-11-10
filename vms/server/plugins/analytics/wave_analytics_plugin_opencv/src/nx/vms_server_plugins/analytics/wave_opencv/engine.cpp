// Copyright 2024. All Rights Reserved.

#include "engine.h"
#include "device_agent.h"

#include <nx/kit/debug.h>
#include <nx/kit/json.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

Engine::Engine()
{
    NX_PRINT << "Wave Analytics Engine created";
}

Engine::~Engine()
{
}

std::string Engine::manifestString() const
{
    // Engine-level manifest with capabilities and type library
    return /*suppress newline*/ 1 + (const char*)
R"json(
{
    "capabilities": "needUncompressedVideoFrames_yuv420",
    "typeLibrary": {
        "eventTypes": [
            {
                "id": "nx.wave_opencv.newTrack",
                "name": "New Object Detected"
            },
            {
                "id": "nx.wave_opencv.loitering",
                "name": "Loitering Detected",
                "flags": "stateDependent"
            },
            {
                "id": "nx.wave_opencv.lineCrossing",
                "name": "Line Crossing"
            },
            {
                "id": "nx.wave_opencv.intrusion",
                "name": "Zone Intrusion",
                "flags": "stateDependent"
            },
            {
                "id": "nx.wave_opencv.fall",
                "name": "Fall Detected"
            },
            {
                "id": "nx.wave_opencv.aggressiveBehavior",
                "name": "Aggressive Behavior"
            },
            {
                "id": "nx.wave_opencv.crowdDensity",
                "name": "Crowd Density Change",
                "flags": "stateDependent"
            },
            {
                "id": "nx.wave_opencv.queueLength",
                "name": "Queue Length Exceeded",
                "flags": "stateDependent"
            },
            {
                "id": "nx.wave_opencv.longWaitTime",
                "name": "Long Wait Time",
                "flags": "stateDependent"
            },
            {
                "id": "nx.wave_opencv.abandonedQueue",
                "name": "Customer Abandoned Queue"
            },
            {
                "id": "nx.wave_opencv.ruleViolation",
                "name": "Business Rule Violation",
                "flags": "stateDependent"
            }
        ],
        "objectTypes": [
            {
                "id": "nx.wave_opencv.person",
                "name": "Person"
            },
            {
                "id": "nx.wave_opencv.vehicle",
                "name": "Vehicle"
            },
            {
                "id": "nx.wave_opencv.car",
                "name": "Car"
            },
            {
                "id": "nx.wave_opencv.truck",
                "name": "Truck"
            },
            {
                "id": "nx.wave_opencv.bus",
                "name": "Bus"
            },
            {
                "id": "nx.wave_opencv.motorcycle",
                "name": "Motorcycle"
            },
            {
                "id": "nx.wave_opencv.bicycle",
                "name": "Bicycle"
            },
            {
                "id": "nx.wave_opencv.animal",
                "name": "Animal"
            }
        ],
        "objectActions": [],
        "enumTypes": []
    }
}
)json";
}

Result<IDeviceAgent*> Engine::doObtainDeviceAgent(const IDeviceInfo* deviceInfo)
{
    return new DeviceAgent(deviceInfo);
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
