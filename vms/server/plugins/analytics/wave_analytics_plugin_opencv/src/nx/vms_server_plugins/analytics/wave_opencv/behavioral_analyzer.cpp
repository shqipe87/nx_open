// Copyright 2024. All Rights Reserved.

#include "behavioral_analyzer.h"

#include <nx/kit/debug.h>
#include <ctime>
#include <cmath>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

BehavioralAnalyzer::BehavioralAnalyzer()
{
}

BehavioralAnalyzer::~BehavioralAnalyzer()
{
}

void BehavioralAnalyzer::initialize(const PluginConfiguration& config)
{
    m_config = config;
    m_zones = config.zones;
    m_tripLines = config.tripLines;

    NX_PRINT << "Behavioral analyzer initialized with "
             << m_zones.size() << " zones and "
             << m_tripLines.size() << " trip lines";
}

void BehavioralAnalyzer::updateZones(const std::vector<Zone>& zones)
{
    m_zones = zones;
}

void BehavioralAnalyzer::updateTripLines(const std::vector<TripLine>& tripLines)
{
    m_tripLines = tripLines;
}

std::vector<AnalyticsEvent> BehavioralAnalyzer::analyze(
    std::vector<TrackedObject>& objects,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    for (auto& object : objects)
    {
        // Update object zone membership
        updateObjectZones(object);

        // Detect loitering
        if (m_config.loiteringTimeSeconds > 0)
        {
            auto loiteringEvents = detectLoitering(object, timestampUs);
            events.insert(events.end(), loiteringEvents.begin(), loiteringEvents.end());
        }

        // Detect line crossings
        auto lineCrossingEvents = detectLineCrossing(object, timestampUs);
        events.insert(events.end(), lineCrossingEvents.begin(), lineCrossingEvents.end());

        // Detect intrusions
        auto intrusionEvents = detectIntrusion(object, timestampUs);
        events.insert(events.end(), intrusionEvents.begin(), intrusionEvents.end());

        // Detect falls (for person objects)
        if (m_config.enableFallDetection && object.detection.className == "person")
        {
            auto fallEvents = detectFall(object, timestampUs);
            events.insert(events.end(), fallEvents.begin(), fallEvents.end());
        }

        // Detect aggressive behavior
        if (m_config.enableAggressiveBehavior)
        {
            auto aggressiveEvents = detectAggressiveBehavior(object, timestampUs);
            events.insert(events.end(), aggressiveEvents.begin(), aggressiveEvents.end());
        }
    }

    return events;
}

void BehavioralAnalyzer::updateObjectZones(TrackedObject& object)
{
    object.currentZones.clear();

    cv::Point2f center = object.detection.center;

    for (const auto& zone : m_zones)
    {
        if (isPointInPolygon(center, zone.polygon))
        {
            object.currentZones.push_back(zone.name);

            // Track entry time if not already in zone
            if (object.zoneEntryTimes.find(zone.name) == object.zoneEntryTimes.end())
            {
                object.zoneEntryTimes[zone.name] = object.lastSeenTimestampUs;
            }
        }
        else
        {
            // Remove entry time if left zone
            object.zoneEntryTimes.erase(zone.name);
        }
    }
}

std::vector<AnalyticsEvent> BehavioralAnalyzer::detectLoitering(
    TrackedObject& object,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    if (!object.isStationary)
    {
        // Reset loitering state if object is moving
        m_loiteringStartTimes.erase(object.trackId);
        m_loiteringAlerted.erase(object.trackId);
        return events;
    }

    // Track loitering start time
    if (m_loiteringStartTimes.find(object.trackId) == m_loiteringStartTimes.end())
    {
        m_loiteringStartTimes[object.trackId] = timestampUs;
    }

    int64_t loiteringDurationUs = timestampUs - m_loiteringStartTimes[object.trackId];
    int loiteringDurationSeconds = loiteringDurationUs / 1000000;

    // Check if loitering threshold exceeded
    int threshold = m_config.loiteringTimeSeconds;

    // Check zone-specific thresholds
    for (const auto& zoneName : object.currentZones)
    {
        for (const auto& zone : m_zones)
        {
            if (zone.name == zoneName && zone.loiteringThresholdSeconds > 0)
            {
                threshold = zone.loiteringThresholdSeconds;
                break;
            }
        }
    }

    if (loiteringDurationSeconds >= threshold)
    {
        // Only alert once per loitering instance
        if (!m_loiteringAlerted[object.trackId])
        {
            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.loitering";
            event.caption = "Loitering Detected";
            event.description = "Object " + object.detection.className +
                              " (ID: " + std::to_string(object.trackId) +
                              ") loitering for " + std::to_string(loiteringDurationSeconds) + " seconds";
            event.timestampUs = timestampUs;
            event.durationUs = loiteringDurationUs;
            event.boundingBox = object.detection.bbox;
            event.trackId = object.trackId;
            event.severity = "medium";
            event.attributes["loitering_duration"] = std::to_string(loiteringDurationSeconds);
            event.attributes["object_type"] = object.detection.className;

            if (!object.currentZones.empty())
            {
                event.attributes["zone"] = object.currentZones[0];
            }

            events.push_back(event);
            m_loiteringAlerted[object.trackId] = true;
        }
    }

    return events;
}

std::vector<AnalyticsEvent> BehavioralAnalyzer::detectLineCrossing(
    TrackedObject& object,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    if (object.trajectory.size() < 2)
        return events;

    cv::Point2f currentPos = object.trajectory.back();
    cv::Point2f previousPos = object.trajectory[object.trajectory.size() - 2];

    for (const auto& line : m_tripLines)
    {
        // Check if trajectory segment crosses the line
        cv::Point p1(previousPos.x, previousPos.y);
        cv::Point p2(currentPos.x, currentPos.y);

        if (doSegmentsIntersect(p1, p2, line.start, line.end))
        {
            // Determine crossing direction
            int orientation1 = lineOrientation(line.start, line.end, p1);
            int orientation2 = lineOrientation(line.start, line.end, p2);

            std::string direction;
            if (orientation1 == 1 && orientation2 == 2)
            {
                direction = line.directionAtoB.empty() ? "A->B" : line.directionAtoB;
            }
            else if (orientation1 == 2 && orientation2 == 1)
            {
                direction = line.directionBtoA.empty() ? "B->A" : line.directionBtoA;
            }
            else
            {
                continue;  // Not a clear crossing
            }

            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.lineCrossing";
            event.caption = "Line Crossing: " + line.name;
            event.description = "Object " + object.detection.className +
                              " (ID: " + std::to_string(object.trackId) +
                              ") crossed line '" + line.name + "' " + direction;
            event.timestampUs = timestampUs;
            event.durationUs = 0;  // Momentary event
            event.boundingBox = object.detection.bbox;
            event.trackId = object.trackId;
            event.severity = "low";
            event.attributes["line_name"] = line.name;
            event.attributes["direction"] = direction;
            event.attributes["object_type"] = object.detection.className;

            events.push_back(event);

            NX_PRINT << "Line crossing detected: " << line.name << " " << direction;
        }
    }

    return events;
}

std::vector<AnalyticsEvent> BehavioralAnalyzer::detectIntrusion(
    TrackedObject& object,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    for (const auto& zone : m_zones)
    {
        if (zone.type != "restricted" && zone.type != "intrusion")
            continue;

        // Check if zone is active at current time
        if (!isZoneActive(zone, timestampUs))
            continue;

        bool inZone = false;
        for (const auto& zoneName : object.currentZones)
        {
            if (zoneName == zone.name)
            {
                inZone = true;
                break;
            }
        }

        if (inZone)
        {
            // Check if we already alerted for this track in this zone
            std::string key = zone.name + "_" + std::to_string(object.trackId);

            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.intrusion";
            event.caption = "Intrusion: " + zone.name;
            event.description = "Object " + object.detection.className +
                              " (ID: " + std::to_string(object.trackId) +
                              ") detected in restricted zone '" + zone.name + "'";
            event.timestampUs = timestampUs;

            // Calculate duration in zone
            auto entryIt = object.zoneEntryTimes.find(zone.name);
            if (entryIt != object.zoneEntryTimes.end())
            {
                event.durationUs = timestampUs - entryIt->second;
            }
            else
            {
                event.durationUs = 0;
            }

            event.boundingBox = object.detection.bbox;
            event.trackId = object.trackId;
            event.severity = "high";
            event.attributes["zone"] = zone.name;
            event.attributes["object_type"] = object.detection.className;

            events.push_back(event);
        }
    }

    return events;
}

std::vector<AnalyticsEvent> BehavioralAnalyzer::detectFall(
    TrackedObject& object,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    // Skip if already detected fall for this track
    if (m_fallDetected[object.trackId])
        return events;

    // Check aspect ratio change (person going from vertical to horizontal)
    float currentAspectRatio = object.aspectRatio;

    // Typical standing person: aspectRatio ~0.4-0.6 (width < height)
    // Fallen person: aspectRatio ~1.5-3.0 (width > height)
    if (currentAspectRatio > 1.2f)
    {
        // Check height decrease
        float avgPreviousHeight = 0;
        float currentHeight = object.heightHistory[object.heightHistoryIndex];

        int count = 0;
        for (int i = 0; i < 5; ++i)  // Check last 5 frames
        {
            int idx = (object.heightHistoryIndex - i - 1 + 10) % 10;
            avgPreviousHeight += object.heightHistory[idx];
            count++;
        }
        avgPreviousHeight /= count;

        float heightDecrease = avgPreviousHeight - currentHeight;
        float heightDecreaseRatio = heightDecrease / avgPreviousHeight;

        // Fall detected if height decreased by > 30% and aspect ratio changed
        if (heightDecreaseRatio > 0.3f)
        {
            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.fall";
            event.caption = "Fall Detected";
            event.description = "Person (ID: " + std::to_string(object.trackId) + ") has fallen";
            event.timestampUs = timestampUs;
            event.durationUs = 0;
            event.boundingBox = object.detection.bbox;
            event.trackId = object.trackId;
            event.severity = "critical";
            event.attributes["aspect_ratio"] = std::to_string(currentAspectRatio);
            event.attributes["height_decrease"] = std::to_string(heightDecrease);

            if (!object.currentZones.empty())
            {
                event.attributes["zone"] = object.currentZones[0];
            }

            events.push_back(event);
            m_fallDetected[object.trackId] = true;

            NX_PRINT << "Fall detected for track " << object.trackId;
        }
    }

    return events;
}

std::vector<AnalyticsEvent> BehavioralAnalyzer::detectAggressiveBehavior(
    TrackedObject& object,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    // Detect rapid movements and sudden direction changes
    float accelerationMag = std::sqrt(
        object.acceleration.x * object.acceleration.x +
        object.acceleration.y * object.acceleration.y
    );

    const float aggressiveAccelerationThreshold = 15.0f;  // Pixels per frame^2

    if (accelerationMag > aggressiveAccelerationThreshold)
    {
        m_rapidChangeCounter[object.trackId]++;

        // Alert if multiple rapid changes in short time
        if (m_rapidChangeCounter[object.trackId] >= 5)
        {
            AnalyticsEvent event;
            event.eventType = "nx.wave_opencv.aggressiveBehavior";
            event.caption = "Aggressive Behavior";
            event.description = "Object " + object.detection.className +
                              " (ID: " + std::to_string(object.trackId) +
                              ") exhibiting aggressive behavior (rapid movements)";
            event.timestampUs = timestampUs;
            event.durationUs = 0;
            event.boundingBox = object.detection.bbox;
            event.trackId = object.trackId;
            event.severity = "high";
            event.attributes["acceleration"] = std::to_string(accelerationMag);
            event.attributes["object_type"] = object.detection.className;

            events.push_back(event);

            m_rapidChangeCounter[object.trackId] = 0;  // Reset counter
        }
    }
    else
    {
        // Decay counter
        if (m_rapidChangeCounter[object.trackId] > 0)
        {
            m_rapidChangeCounter[object.trackId]--;
        }
    }

    return events;
}

bool BehavioralAnalyzer::isZoneActive(const Zone& zone, int64_t timestampUs) const
{
    if (zone.startHour == 0 && zone.endHour == 24)
        return true;  // Always active

    // Convert timestamp to local time
    time_t timeSeconds = timestampUs / 1000000;
    struct tm* timeInfo = localtime(&timeSeconds);
    int currentHour = timeInfo->tm_hour;

    if (zone.startHour <= zone.endHour)
    {
        return currentHour >= zone.startHour && currentHour < zone.endHour;
    }
    else
    {
        // Wraps around midnight
        return currentHour >= zone.startHour || currentHour < zone.endHour;
    }
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
