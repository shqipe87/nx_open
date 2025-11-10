// Copyright 2024. All Rights Reserved.
//
// Behavioral analytics: loitering, line crossing, intrusion, fall detection, aggressive behavior

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
 * Analyzes object behavior and generates events.
 */
class BehavioralAnalyzer
{
public:
    BehavioralAnalyzer();
    ~BehavioralAnalyzer();

    /**
     * Initialize with configuration.
     */
    void initialize(const PluginConfiguration& config);

    /**
     * Analyze tracked objects and generate behavioral events.
     */
    std::vector<AnalyticsEvent> analyze(
        std::vector<TrackedObject>& objects,
        int64_t timestampUs);

    /**
     * Update zones configuration.
     */
    void updateZones(const std::vector<Zone>& zones);

    /**
     * Update trip lines configuration.
     */
    void updateTripLines(const std::vector<TripLine>& tripLines);

private:
    /**
     * Detect loitering (stationary for extended period).
     */
    std::vector<AnalyticsEvent> detectLoitering(
        TrackedObject& object,
        int64_t timestampUs);

    /**
     * Detect line crossings.
     */
    std::vector<AnalyticsEvent> detectLineCrossing(
        TrackedObject& object,
        int64_t timestampUs);

    /**
     * Detect zone intrusions.
     */
    std::vector<AnalyticsEvent> detectIntrusion(
        TrackedObject& object,
        int64_t timestampUs);

    /**
     * Detect falls (person orientation change).
     */
    std::vector<AnalyticsEvent> detectFall(
        TrackedObject& object,
        int64_t timestampUs);

    /**
     * Detect aggressive behavior (rapid movements, sudden changes).
     */
    std::vector<AnalyticsEvent> detectAggressiveBehavior(
        TrackedObject& object,
        int64_t timestampUs);

    /**
     * Update object zone membership.
     */
    void updateObjectZones(TrackedObject& object);

    /**
     * Check if time is within zone active hours.
     */
    bool isZoneActive(const Zone& zone, int64_t timestampUs) const;

private:
    PluginConfiguration m_config;
    std::vector<Zone> m_zones;
    std::vector<TripLine> m_tripLines;

    // Track line crossing state
    struct LineCrossingState
    {
        int trackId;
        bool wasAboveLine;
        cv::Point2f lastPosition;
    };
    std::map<int, std::map<std::string, LineCrossingState>> m_lineCrossingStates;

    // Track loitering state
    std::map<int, int64_t> m_loiteringStartTimes;
    std::map<int, bool> m_loiteringAlerted;

    // Track fall detection state
    std::map<int, bool> m_fallDetected;

    // Track aggressive behavior state
    std::map<int, int> m_rapidChangeCounter;
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
