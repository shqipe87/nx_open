// Copyright 2024. All Rights Reserved.
//
// Plugin entry point for Wave Analytics Plugin with OpenCV

#include <nx/sdk/analytics/i_integration.h>
#include <nx/sdk/analytics/helpers/plugin.h>

#include "../src/nx/vms_server_plugins/analytics/wave_opencv/integration.h"

/**
 * Plugin entry point.
 * Called by the Server when the plugin is loaded.
 * Must return an IIntegration instance.
 */
extern "C" NX_PLUGIN_API nx::sdk::IIntegration* createNxPlugin()
{
    return new nx::vms_server_plugins::analytics::wave_opencv::Integration();
}
