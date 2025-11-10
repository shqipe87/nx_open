// Copyright 2024. All Rights Reserved.
//
// Wave Analytics Plugin with OpenCV
// Main Integration class implementing IIntegration interface

#pragma once

#include <nx/sdk/analytics/helpers/integration.h>
#include <nx/sdk/analytics/i_engine.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Main plugin class (factory) implementing IIntegration interface.
 * Creates Engine instances for each plugin configuration.
 */
class Integration: public nx::sdk::analytics::Integration
{
public:
    Integration();
    virtual ~Integration() override;

protected:
    /**
     * Called once when plugin is loaded by the Server.
     * Returns plugin-level manifest with metadata.
     */
    virtual std::string manifestString() const override;

    /**
     * Factory method to create Engine instances.
     * Each Engine represents a plugin instance with its own settings.
     */
    virtual nx::sdk::Result<nx::sdk::analytics::IEngine*> doObtainEngine() override;
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
