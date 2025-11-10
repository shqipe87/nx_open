// Copyright 2024. All Rights Reserved.
//
// Configuration loader for Wave Analytics Plugin

#pragma once

#include "types.h"
#include <string>
#include <fstream>
#include <vector>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Configuration loader and parser.
 * Loads configuration from JSON files or Wave SDK settings.
 */
class ConfigurationLoader
{
public:
    ConfigurationLoader();
    ~ConfigurationLoader();

    /**
     * Load configuration from JSON file.
     */
    static PluginConfiguration loadFromFile(const std::string& filePath);

    /**
     * Load configuration from JSON string.
     */
    static PluginConfiguration loadFromString(const std::string& jsonContent);

    /**
     * Save configuration to JSON file.
     */
    static bool saveToFile(const PluginConfiguration& config, const std::string& filePath);

    /**
     * Convert configuration to JSON string.
     */
    static std::string toJsonString(const PluginConfiguration& config);

    /**
     * Validate configuration.
     */
    static bool validate(const PluginConfiguration& config, std::string& errorMessage);

    /**
     * Get default configuration file paths to search.
     */
    static std::vector<std::string> getDefaultConfigPaths();

private:
    /**
     * Parse detection settings.
     */
    static void parseDetectionSettings(const std::string& json, PluginConfiguration& config);

    /**
     * Parse analytics settings.
     */
    static void parseAnalyticsSettings(const std::string& json, PluginConfiguration& config);

    /**
     * Parse zones.
     */
    static std::vector<Zone> parseZones(const std::string& json);

    /**
     * Parse trip lines.
     */
    static std::vector<TripLine> parseTripLines(const std::string& json);

    /**
     * Parse business rules.
     */
    static std::vector<BusinessRule> parseRules(const std::string& json);

    /**
     * Simple JSON value extractor (for basic parsing without external libs).
     */
    static std::string getJsonValue(const std::string& json, const std::string& key);
    static int getJsonInt(const std::string& json, const std::string& key, int defaultValue);
    static float getJsonFloat(const std::string& json, const std::string& key, float defaultValue);
    static bool getJsonBool(const std::string& json, const std::string& key, bool defaultValue);
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
