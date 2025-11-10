// Copyright 2024. All Rights Reserved.

#include "queue_manager.h"

#include <nx/kit/debug.h>
#include <algorithm>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

QueueManager::QueueManager()
    : m_lastMetricsUpdateUs(0)
    , m_metricsUpdateIntervalUs(1000000)  // Update every 1 second
    , m_queueLengthThreshold(10)
    , m_waitTimeThresholdSeconds(300.0f)  // 5 minutes
    , m_abandonmentRateThreshold(0.2f)    // 20%
{
}

QueueManager::~QueueManager()
{
}

void QueueManager::initialize(const PluginConfiguration& config)
{
    m_config = config;
    NX_PRINT << "Queue manager initialized";
}

std::vector<AnalyticsEvent> QueueManager::update(
    const std::vector<TrackedObject>& objects,
    int64_t timestampUs)
{
    if (!m_config.enableQueueAnalytics)
        return {};

    // Update customer journeys
    updateCustomerJourneys(objects, timestampUs);

    // Calculate metrics periodically
    if (timestampUs - m_lastMetricsUpdateUs >= m_metricsUpdateIntervalUs)
    {
        calculateMetrics();
        m_lastMetricsUpdateUs = timestampUs;
    }

    // Generate events
    auto events = generateQueueEvents(timestampUs);

    // Cleanup old journeys
    cleanupOldJourneys(timestampUs);

    return events;
}

void QueueManager::updateCustomerJourneys(
    const std::vector<TrackedObject>& objects,
    int64_t timestampUs)
{
    for (const auto& object : objects)
    {
        // Only track persons in queue analytics
        if (object.detection.className != "person")
            continue;

        int trackId = object.trackId;

        // Create journey if doesn't exist
        if (m_customerJourneys.find(trackId) == m_customerJourneys.end())
        {
            CustomerJourney journey;
            journey.trackId = trackId;
            journey.state = CustomerState::Entering;
            journey.enterTimestampUs = timestampUs;
            journey.waitStartTimestampUs = 0;
            journey.serviceStartTimestampUs = 0;
            journey.exitTimestampUs = 0;
            journey.queuePosition = -1;

            m_customerJourneys[trackId] = journey;
        }

        CustomerJourney& journey = m_customerJourneys[trackId];

        // Determine current state based on zones
        CustomerState newState = determineCustomerState(object);

        // Update state transitions
        if (newState != journey.state)
        {
            CustomerState previousState = journey.state;
            journey.state = newState;

            // Handle state transitions
            if (newState == CustomerState::Waiting && previousState == CustomerState::Entering)
            {
                journey.waitStartTimestampUs = timestampUs;
            }
            else if (newState == CustomerState::BeingServed && previousState == CustomerState::Waiting)
            {
                journey.serviceStartTimestampUs = timestampUs;

                // Record wait time
                if (journey.waitStartTimestampUs > 0)
                {
                    float waitTime = (timestampUs - journey.waitStartTimestampUs) / 1000000.0f;
                    m_waitTimesLastHour.push_back(waitTime);
                    m_waitTimeTimestamps.push_back(timestampUs);
                }
            }
            else if (newState == CustomerState::Exited && previousState == CustomerState::BeingServed)
            {
                journey.exitTimestampUs = timestampUs;

                // Record service time
                if (journey.serviceStartTimestampUs > 0)
                {
                    float serviceTime = (timestampUs - journey.serviceStartTimestampUs) / 1000000.0f;
                    m_serviceTimesLastHour.push_back(serviceTime);
                    m_serviceTimeTimestamps.push_back(timestampUs);
                }

                // Increment served counter
                time_t timeSeconds = timestampUs / 1000000;
                struct tm* timeInfo = localtime(&timeSeconds);
                int hour = timeInfo->tm_hour;
                m_hourlyCustomerCounts[hour]++;
            }
            else if (newState == CustomerState::Abandoned)
            {
                // Customer left before being served
                time_t timeSeconds = timestampUs / 1000000;
                struct tm* timeInfo = localtime(&timeSeconds);
                int hour = timeInfo->tm_hour;
                m_hourlyAbandonmentCounts[hour]++;
            }
        }
    }

    // Detect customers who have left (no longer in objects list)
    std::vector<int> tracksToRemove;
    for (auto& pair : m_customerJourneys)
    {
        int trackId = pair.first;
        bool found = false;

        for (const auto& object : objects)
        {
            if (object.trackId == trackId)
            {
                found = true;
                break;
            }
        }

        if (!found && pair.second.state != CustomerState::Exited)
        {
            // Track disappeared - check if abandoned
            if (pair.second.state == CustomerState::Waiting)
            {
                pair.second.state = CustomerState::Abandoned;
                pair.second.exitTimestampUs = timestampUs;
            }
            else
            {
                pair.second.state = CustomerState::Exited;
                pair.second.exitTimestampUs = timestampUs;
            }
        }
    }
}

CustomerState QueueManager::determineCustomerState(const TrackedObject& object) const
{
    // Determine state based on which zone the person is in
    for (const auto& zoneName : object.currentZones)
    {
        // Find the zone
        for (const auto& zone : m_config.zones)
        {
            if (zone.name == zoneName)
            {
                if (zone.type == "queue_entrance")
                    return CustomerState::Entering;
                else if (zone.type == "queue_waiting")
                    return CustomerState::Waiting;
                else if (zone.type == "queue_service")
                    return CustomerState::BeingServed;
                else if (zone.type == "queue_exit")
                    return CustomerState::Exited;
            }
        }
    }

    // Default to current state if no zone match
    auto it = m_customerJourneys.find(object.trackId);
    return (it != m_customerJourneys.end()) ? it->second.state : CustomerState::Entering;
}

void QueueManager::calculateMetrics()
{
    // Calculate current queue length
    m_currentMetrics.currentQueueLength = 0;
    for (const auto& pair : m_customerJourneys)
    {
        if (pair.second.state == CustomerState::Waiting)
        {
            m_currentMetrics.currentQueueLength++;
        }
    }

    // Calculate average wait time (last hour)
    if (!m_waitTimesLastHour.empty())
    {
        float sum = 0;
        for (float time : m_waitTimesLastHour)
        {
            sum += time;
        }
        m_currentMetrics.averageWaitTimeSeconds = sum / m_waitTimesLastHour.size();
    }
    else
    {
        m_currentMetrics.averageWaitTimeSeconds = 0;
    }

    // Calculate average service time
    if (!m_serviceTimesLastHour.empty())
    {
        float sum = 0;
        for (float time : m_serviceTimesLastHour)
        {
            sum += time;
        }
        m_currentMetrics.averageServiceTimeSeconds = sum / m_serviceTimesLastHour.size();
    }
    else
    {
        m_currentMetrics.averageServiceTimeSeconds = 0;
    }

    // Calculate customers served last hour
    time_t currentTime = time(nullptr);
    struct tm* timeInfo = localtime(&currentTime);
    int currentHour = timeInfo->tm_hour;
    m_currentMetrics.customersServedLastHour = m_hourlyCustomerCounts[currentHour];
    m_currentMetrics.abandonedLastHour = m_hourlyAbandonmentCounts[currentHour];

    // Calculate abandonment rate
    int totalCustomers = m_currentMetrics.customersServedLastHour + m_currentMetrics.abandonedLastHour;
    if (totalCustomers > 0)
    {
        m_currentMetrics.abandonmentRate =
            (float)m_currentMetrics.abandonedLastHour / totalCustomers;
    }
    else
    {
        m_currentMetrics.abandonmentRate = 0;
    }
}

std::vector<AnalyticsEvent> QueueManager::generateQueueEvents(int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    // Event: Queue length exceeded
    if (m_currentMetrics.currentQueueLength >= m_queueLengthThreshold)
    {
        if (!m_alertStates["queue_length_exceeded"])
        {
            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.queueLength";
            event.caption = "Queue Length Exceeded";
            event.description = "Queue length (" +
                              std::to_string(m_currentMetrics.currentQueueLength) +
                              ") exceeded threshold (" +
                              std::to_string(m_queueLengthThreshold) + ")";
            event.timestampUs = timestampUs;
            event.durationUs = 0;
            event.trackId = -1;
            event.severity = "high";
            event.attributes["queue_length"] = std::to_string(m_currentMetrics.currentQueueLength);
            event.attributes["threshold"] = std::to_string(m_queueLengthThreshold);

            events.push_back(event);
            m_alertStates["queue_length_exceeded"] = true;
        }
    }
    else
    {
        m_alertStates["queue_length_exceeded"] = false;
    }

    // Event: Long wait time
    if (m_currentMetrics.averageWaitTimeSeconds >= m_waitTimeThresholdSeconds)
    {
        if (!m_alertStates["long_wait_time"])
        {
            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.longWaitTime";
            event.caption = "Long Wait Time";
            event.description = "Average wait time (" +
                              std::to_string((int)m_currentMetrics.averageWaitTimeSeconds) +
                              "s) exceeded threshold (" +
                              std::to_string((int)m_waitTimeThresholdSeconds) + "s)";
            event.timestampUs = timestampUs;
            event.durationUs = 0;
            event.trackId = -1;
            event.severity = "medium";
            event.attributes["avg_wait_time"] =
                std::to_string(m_currentMetrics.averageWaitTimeSeconds);
            event.attributes["threshold"] = std::to_string(m_waitTimeThresholdSeconds);

            events.push_back(event);
            m_alertStates["long_wait_time"] = true;
        }
    }
    else
    {
        m_alertStates["long_wait_time"] = false;
    }

    // Event: Customer abandoned queue
    for (const auto& pair : m_customerJourneys)
    {
        if (pair.second.state == CustomerState::Abandoned &&
            pair.second.exitTimestampUs == timestampUs)
        {
            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.abandonedQueue";
            event.caption = "Customer Abandoned Queue";
            event.description = "Customer (ID: " + std::to_string(pair.first) +
                              ") abandoned queue after waiting";
            event.timestampUs = timestampUs;
            event.durationUs = 0;
            event.trackId = pair.first;
            event.severity = "medium";

            if (pair.second.waitStartTimestampUs > 0)
            {
                float waitTime = (timestampUs - pair.second.waitStartTimestampUs) / 1000000.0f;
                event.attributes["wait_time"] = std::to_string(waitTime);
            }

            events.push_back(event);
        }
    }

    return events;
}

void QueueManager::cleanupOldJourneys(int64_t timestampUs)
{
    const int64_t maxAgeUs = 3600 * 1000000;  // 1 hour

    auto it = m_customerJourneys.begin();
    while (it != m_customerJourneys.end())
    {
        if (it->second.state == CustomerState::Exited ||
            it->second.state == CustomerState::Abandoned)
        {
            if (timestampUs - it->second.exitTimestampUs > maxAgeUs)
            {
                it = m_customerJourneys.erase(it);
                continue;
            }
        }
        ++it;
    }

    // Cleanup old wait/service times (keep last hour)
    const int64_t oneHourUs = 3600 * 1000000;

    while (!m_waitTimeTimestamps.empty() &&
           timestampUs - m_waitTimeTimestamps.front() > oneHourUs)
    {
        m_waitTimeTimestamps.pop_front();
        m_waitTimesLastHour.pop_front();
    }

    while (!m_serviceTimeTimestamps.empty() &&
           timestampUs - m_serviceTimeTimestamps.front() > oneHourUs)
    {
        m_serviceTimeTimestamps.pop_front();
        m_serviceTimesLastHour.pop_front();
    }
}

QueueMetrics QueueManager::getMetrics() const
{
    return m_currentMetrics;
}

int QueueManager::getQueueLength(const std::string& queueName) const
{
    return m_currentMetrics.currentQueueLength;
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
