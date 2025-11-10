// Copyright 2024. All Rights Reserved.
//
// Queue management and analytics

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <deque>

#include "types.h"

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Customer state in queue.
 */
enum class CustomerState
{
    Entering,       // In entrance zone
    Waiting,        // In queue waiting
    BeingServed,    // In service zone
    Exited,         // Left the system
    Abandoned       // Left before being served
};

/**
 * Customer journey tracking.
 */
struct CustomerJourney
{
    int trackId;
    CustomerState state;
    int64_t enterTimestampUs;
    int64_t waitStartTimestampUs;
    int64_t serviceStartTimestampUs;
    int64_t exitTimestampUs;
    int queuePosition;
};

/**
 * Manages queue analytics and customer journeys.
 */
class QueueManager
{
public:
    QueueManager();
    ~QueueManager();

    /**
     * Initialize with configuration.
     */
    void initialize(const PluginConfiguration& config);

    /**
     * Update queue state with tracked objects.
     */
    std::vector<AnalyticsEvent> update(
        const std::vector<TrackedObject>& objects,
        int64_t timestampUs);

    /**
     * Get current queue metrics.
     */
    QueueMetrics getMetrics() const;

    /**
     * Get queue length for a specific queue zone.
     */
    int getQueueLength(const std::string& queueName) const;

private:
    /**
     * Update customer journeys.
     */
    void updateCustomerJourneys(
        const std::vector<TrackedObject>& objects,
        int64_t timestampUs);

    /**
     * Determine customer state based on zones.
     */
    CustomerState determineCustomerState(const TrackedObject& object) const;

    /**
     * Find zone by name and type.
     */
    const Zone* findZone(const std::string& name, const std::string& type) const;

    /**
     * Calculate queue metrics.
     */
    void calculateMetrics();

    /**
     * Generate queue-related events.
     */
    std::vector<AnalyticsEvent> generateQueueEvents(int64_t timestampUs);

    /**
     * Clean up old journey records.
     */
    void cleanupOldJourneys(int64_t timestampUs);

private:
    PluginConfiguration m_config;

    // Customer journey tracking
    std::map<int, CustomerJourney> m_customerJourneys;

    // Queue metrics
    QueueMetrics m_currentMetrics;

    // Historical data for metrics calculation
    std::deque<float> m_waitTimesLastHour;
    std::deque<float> m_serviceTimesLastHour;
    std::deque<int64_t> m_waitTimeTimestamps;
    std::deque<int64_t> m_serviceTimeTimestamps;

    // Hourly statistics
    std::map<int, int> m_hourlyCustomerCounts;
    std::map<int, int> m_hourlyAbandonmentCounts;

    // Alert state tracking
    std::map<std::string, bool> m_alertStates;
    int64_t m_lastMetricsUpdateUs;
    int64_t m_metricsUpdateIntervalUs;

    // Configurable thresholds
    int m_queueLengthThreshold;
    float m_waitTimeThresholdSeconds;
    float m_abandonmentRateThreshold;
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
