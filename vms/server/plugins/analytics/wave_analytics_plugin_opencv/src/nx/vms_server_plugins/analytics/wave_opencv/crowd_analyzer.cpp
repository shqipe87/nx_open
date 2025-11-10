// Copyright 2024. All Rights Reserved.

#include "crowd_analyzer.h"

#include <nx/kit/debug.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

CrowdAnalyzer::CrowdAnalyzer()
    : m_heatmapFrameCount(0)
    , m_heatmapUpdateInterval(30)  // Update heatmap every 30 frames
{
}

CrowdAnalyzer::~CrowdAnalyzer()
{
}

void CrowdAnalyzer::initialize(const PluginConfiguration& config)
{
    m_config = config;
    NX_PRINT << "Crowd analyzer initialized";
}

std::vector<AnalyticsEvent> CrowdAnalyzer::analyze(
    const std::vector<TrackedObject>& objects,
    const cv::Size& frameSize,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    if (!m_config.enableCrowdDensity)
        return events;

    // Calculate zone density
    calculateZoneDensity(objects);

    // Generate events for density changes
    events = generateDensityEvents(timestampUs);

    return events;
}

void CrowdAnalyzer::calculateZoneDensity(const std::vector<TrackedObject>& objects)
{
    // Reset density counts
    m_zoneDensity.clear();

    // Count objects in each zone
    for (const auto& object : objects)
    {
        for (const auto& zoneName : object.currentZones)
        {
            m_zoneDensity[zoneName]++;
        }
    }

    // Update density levels
    for (const auto& pair : m_zoneDensity)
    {
        const std::string& zoneName = pair.first;
        int count = pair.second;

        m_previousDensityLevel[zoneName] = m_zoneDensityLevel[zoneName];
        m_zoneDensityLevel[zoneName] = determineDensityLevel(count);
    }
}

std::string CrowdAnalyzer::determineDensityLevel(int objectCount) const
{
    if (objectCount >= m_config.crowdCriticalThreshold)
        return "critical";
    else if (objectCount >= m_config.crowdHighThreshold)
        return "high";
    else if (objectCount >= m_config.crowdMediumThreshold)
        return "medium";
    else if (objectCount >= m_config.crowdLowThreshold)
        return "low";
    else
        return "normal";
}

std::vector<AnalyticsEvent> CrowdAnalyzer::generateDensityEvents(int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    for (const auto& pair : m_zoneDensityLevel)
    {
        const std::string& zoneName = pair.first;
        const std::string& currentLevel = pair.second;
        const std::string& previousLevel = m_previousDensityLevel[zoneName];

        // Generate event if density level changed to medium or higher
        if (currentLevel != previousLevel && currentLevel != "normal" && currentLevel != "low")
        {
            std::string severity = "low";
            if (currentLevel == "critical")
                severity = "critical";
            else if (currentLevel == "high")
                severity = "high";
            else
                severity = "medium";

            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.crowdDensity";
            event.caption = "Crowd Density: " + currentLevel;
            event.description = "Crowd density in zone '" + zoneName +
                              "' changed to " + currentLevel +
                              " (" + std::to_string(m_zoneDensity[zoneName]) + " people)";
            event.timestampUs = timestampUs;
            event.durationUs = 0;  // State-dependent event
            event.trackId = -1;
            event.severity = severity;
            event.attributes["zone"] = zoneName;
            event.attributes["density_level"] = currentLevel;
            event.attributes["object_count"] = std::to_string(m_zoneDensity[zoneName]);

            events.push_back(event);

            NX_PRINT << "Crowd density change in " << zoneName << ": " << currentLevel;
        }
    }

    return events;
}

cv::Mat CrowdAnalyzer::generateHeatmap(
    const std::vector<TrackedObject>& objects,
    const cv::Size& frameSize)
{
    // Initialize accumulator if needed
    if (m_heatmapAccumulator.empty() || m_heatmapAccumulator.size() != frameSize)
    {
        m_heatmapAccumulator = cv::Mat::zeros(frameSize, CV_32F);
        m_heatmapFrameCount = 0;
    }

    // Add current object positions to accumulator
    for (const auto& object : objects)
    {
        cv::Point center(object.detection.center.x, object.detection.center.y);

        // Draw a Gaussian blob around each object
        int radius = 50;  // Influence radius
        cv::circle(m_heatmapAccumulator, center, radius, cv::Scalar(1.0), -1, cv::LINE_AA);
    }

    m_heatmapFrameCount++;

    // Generate heatmap visualization
    cv::Mat normalized;
    cv::normalize(m_heatmapAccumulator, normalized, 0, 255, cv::NORM_MINMAX);
    normalized.convertTo(normalized, CV_8U);

    // Apply colormap
    cv::Mat heatmap;
    cv::applyColorMap(normalized, heatmap, cv::COLORMAP_JET);

    // Reset accumulator periodically
    if (m_heatmapFrameCount >= m_heatmapUpdateInterval * 100)
    {
        m_heatmapAccumulator *= 0.9;  // Decay old data
        m_heatmapFrameCount = 0;
    }

    return heatmap;
}

std::string CrowdAnalyzer::getDensityLevel(const std::string& zoneName) const
{
    auto it = m_zoneDensityLevel.find(zoneName);
    return (it != m_zoneDensityLevel.end()) ? it->second : "normal";
}

int CrowdAnalyzer::getZoneObjectCount(const std::string& zoneName) const
{
    auto it = m_zoneDensity.find(zoneName);
    return (it != m_zoneDensity.end()) ? it->second : 0;
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
