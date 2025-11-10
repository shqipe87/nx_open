// Copyright 2024. All Rights Reserved.
//
// Crowd density analysis and heatmap generation

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>

#include "types.h"

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Analyzes crowd density and generates heatmaps.
 */
class CrowdAnalyzer
{
public:
    CrowdAnalyzer();
    ~CrowdAnalyzer();

    /**
     * Initialize with configuration.
     */
    void initialize(const PluginConfiguration& config);

    /**
     * Analyze crowd density and generate events.
     */
    std::vector<AnalyticsEvent> analyze(
        const std::vector<TrackedObject>& objects,
        const cv::Size& frameSize,
        int64_t timestampUs);

    /**
     * Generate density heatmap.
     */
    cv::Mat generateHeatmap(
        const std::vector<TrackedObject>& objects,
        const cv::Size& frameSize);

    /**
     * Get current density level for a zone.
     */
    std::string getDensityLevel(const std::string& zoneName) const;

    /**
     * Get object count in a zone.
     */
    int getZoneObjectCount(const std::string& zoneName) const;

private:
    /**
     * Calculate density for each zone.
     */
    void calculateZoneDensity(const std::vector<TrackedObject>& objects);

    /**
     * Determine density level based on count.
     */
    std::string determineDensityLevel(int objectCount) const;

    /**
     * Generate density change events.
     */
    std::vector<AnalyticsEvent> generateDensityEvents(int64_t timestampUs);

private:
    PluginConfiguration m_config;

    // Zone density tracking
    std::map<std::string, int> m_zoneDensity;
    std::map<std::string, std::string> m_zoneDensityLevel;
    std::map<std::string, std::string> m_previousDensityLevel;

    // Heatmap accumulator
    cv::Mat m_heatmapAccumulator;
    int m_heatmapFrameCount;
    int m_heatmapUpdateInterval; // Frames between heatmap updates
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
