// Copyright 2024. All Rights Reserved.
//
// Wave Analytics Plugin with OpenCV
// Engine class managing analytics instances

#pragma once

#include <nx/sdk/analytics/helpers/engine.h>
#include <nx/sdk/analytics/i_uncompressed_video_frame.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Engine class implementing IEngine interface.
 * Manages plugin instance with specific settings and creates DeviceAgents.
 */
class Engine: public nx::sdk::analytics::Engine
{
public:
    Engine();
    virtual ~Engine() override;

protected:
    /**
     * Returns Engine-level manifest with capabilities and type library.
     */
    virtual std::string manifestString() const override;

    /**
     * Factory method to create DeviceAgent for each camera/device.
     */
    virtual nx::sdk::Result<nx::sdk::analytics::IDeviceAgent*> doObtainDeviceAgent(
        const nx::sdk::IDeviceInfo* deviceInfo) override;
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
