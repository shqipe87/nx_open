// Copyright 2024. All Rights Reserved.

#include "device_agent.h"

#include <nx/kit/debug.h>
#include <nx/kit/json.h>
#include <nx/sdk/analytics/helpers/object_metadata.h>
#include <nx/sdk/analytics/helpers/object_metadata_packet.h>
#include <nx/sdk/analytics/helpers/event_metadata.h>
#include <nx/sdk/analytics/helpers/event_metadata_packet.h>
#include <nx/sdk/helpers/string.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

DeviceAgent::DeviceAgent(const IDeviceInfo* deviceInfo)
    : ConsumingDeviceAgent(deviceInfo, /*enableOutput*/ true)
{
    m_deviceId = deviceInfo->id();
    NX_PRINT << "DeviceAgent created for device: " << nx::kit::utils::toString(m_deviceId);
}

DeviceAgent::~DeviceAgent()
{
}

std::string DeviceAgent::manifestString() const
{
    // DeviceAgent-level manifest
    return /*suppress newline*/ 1 + (const char*)
R"json(
{
    "supportedTypes": [
        {"objectTypeId": "nx.wave_opencv.person"},
        {"objectTypeId": "nx.wave_opencv.vehicle"},
        {"objectTypeId": "nx.wave_opencv.car"},
        {"objectTypeId": "nx.wave_opencv.truck"},
        {"objectTypeId": "nx.wave_opencv.bus"},
        {"objectTypeId": "nx.wave_opencv.motorcycle"},
        {"objectTypeId": "nx.wave_opencv.bicycle"},
        {"objectTypeId": "nx.wave_opencv.animal"},
        {"eventTypeId": "nx.wave_opencv.newTrack"},
        {"eventTypeId": "nx.wave_opencv.loitering"},
        {"eventTypeId": "nx.wave_opencv.lineCrossing"},
        {"eventTypeId": "nx.wave_opencv.intrusion"},
        {"eventTypeId": "nx.wave_opencv.fall"},
        {"eventTypeId": "nx.wave_opencv.aggressiveBehavior"},
        {"eventTypeId": "nx.wave_opencv.crowdDensity"},
        {"eventTypeId": "nx.wave_opencv.queueLength"},
        {"eventTypeId": "nx.wave_opencv.longWaitTime"},
        {"eventTypeId": "nx.wave_opencv.abandonedQueue"},
        {"eventTypeId": "nx.wave_opencv.ruleViolation"}
    ]
}
)json";
}

bool DeviceAgent::pushUncompressedVideoFrame(const IUncompressedVideoFrame* videoFrame)
{
    if (!videoFrame)
        return false;

    // Initialize components on first frame
    if (!m_initialized)
    {
        loadConfiguration();

        m_detector = std::make_unique<ObjectDetector>();
        if (!m_detector->initialize(m_config))
        {
            NX_PRINT << "Failed to initialize object detector";
            return false;
        }

        m_tracker = std::make_unique<ObjectTracker>();
        m_tracker->initialize(m_config);

        m_behavioralAnalyzer = std::make_unique<BehavioralAnalyzer>();
        m_behavioralAnalyzer->initialize(m_config);

        m_crowdAnalyzer = std::make_unique<CrowdAnalyzer>();
        m_crowdAnalyzer->initialize(m_config);

        m_queueManager = std::make_unique<QueueManager>();
        m_queueManager->initialize(m_config);

        m_rulesEngine = std::make_unique<RulesEngine>();
        m_rulesEngine->initialize(m_config);

        m_initialized = true;
        NX_PRINT << "Analytics pipeline initialized successfully";
    }

    // Frame skipping for performance
    if (++m_frameSkipCounter < m_config.frameSkipRate)
    {
        return true;
    }
    m_frameSkipCounter = 0;

    try
    {
        processVideoFrame(videoFrame);
    }
    catch (const std::exception& e)
    {
        NX_PRINT << "Exception processing frame: " << e.what();
        return false;
    }

    return true;
}

void DeviceAgent::processVideoFrame(const IUncompressedVideoFrame* videoFrame)
{
    int64_t timestampUs = videoFrame->timestampUs();

    // Convert frame to OpenCV Mat
    cv::Mat frame = convertFrameToMat(videoFrame);
    if (frame.empty())
    {
        NX_PRINT << "Failed to convert frame to Mat";
        return;
    }

    // Step 1: Object Detection
    std::vector<Detection> detections = m_detector->detect(frame);

    // Step 2: Object Tracking
    std::vector<TrackedObject> trackedObjects = m_tracker->update(detections, timestampUs);

    // Generate object metadata
    generateObjectMetadata(trackedObjects, timestampUs);

    // Step 3: Behavioral Analytics
    std::vector<AnalyticsEvent> behavioralEvents =
        m_behavioralAnalyzer->analyze(trackedObjects, timestampUs);

    // Step 4: Crowd Density Analysis
    std::vector<AnalyticsEvent> crowdEvents =
        m_crowdAnalyzer->analyze(trackedObjects, frame.size(), timestampUs);

    // Step 5: Queue Management
    std::vector<AnalyticsEvent> queueEvents =
        m_queueManager->update(trackedObjects, timestampUs);

    // Step 6: Business Rules Evaluation
    std::vector<AnalyticsEvent> ruleEvents =
        m_rulesEngine->evaluate(trackedObjects, timestampUs);

    // Combine all events
    std::vector<AnalyticsEvent> allEvents;
    allEvents.insert(allEvents.end(), behavioralEvents.begin(), behavioralEvents.end());
    allEvents.insert(allEvents.end(), crowdEvents.begin(), crowdEvents.end());
    allEvents.insert(allEvents.end(), queueEvents.begin(), queueEvents.end());
    allEvents.insert(allEvents.end(), ruleEvents.begin(), ruleEvents.end());

    // Generate event metadata
    generateEventMetadata(allEvents, timestampUs);

    m_frameNumber++;
    m_lastProcessedTimestampUs = timestampUs;
}

cv::Mat DeviceAgent::convertFrameToMat(const IUncompressedVideoFrame* videoFrame)
{
    const int width = videoFrame->width();
    const int height = videoFrame->height();

    // Get YUV420 planes
    const uint8_t* yPlane = (const uint8_t*)videoFrame->data(0);
    const uint8_t* uPlane = (const uint8_t*)videoFrame->data(1);
    const uint8_t* vPlane = (const uint8_t*)videoFrame->data(2);

    if (!yPlane || !uPlane || !vPlane)
    {
        NX_PRINT << "Invalid video frame data";
        return cv::Mat();
    }

    // Create YUV Mat
    cv::Mat yuvFrame(height + height / 2, width, CV_8UC1);

    // Copy Y plane
    memcpy(yuvFrame.data, yPlane, width * height);

    // Copy U and V planes (interleaved for NV12 format)
    uint8_t* uvDest = yuvFrame.data + width * height;
    const int uvWidth = width / 2;
    const int uvHeight = height / 2;

    for (int i = 0; i < uvHeight; ++i)
    {
        for (int j = 0; j < uvWidth; ++j)
        {
            uvDest[i * width + j * 2] = uPlane[i * uvWidth + j];
            uvDest[i * width + j * 2 + 1] = vPlane[i * uvWidth + j];
        }
    }

    // Convert YUV420 to BGR
    cv::Mat bgrFrame;
    cv::cvtColor(yuvFrame, bgrFrame, cv::COLOR_YUV2BGR_NV12);

    return bgrFrame;
}

void DeviceAgent::generateObjectMetadata(
    const std::vector<TrackedObject>& objects,
    int64_t timestampUs)
{
    if (objects.empty())
        return;

    auto metadataPacket = makePtr<ObjectMetadataPacket>();
    metadataPacket->setTimestampUs(timestampUs);

    for (const auto& object : objects)
    {
        auto objectMetadata = makePtr<ObjectMetadata>();

        // Set object type ID
        std::string typeId = "nx.wave_opencv." + object.detection.className;
        objectMetadata->setTypeId(typeId);

        // Set track ID
        objectMetadata->setTrackId(nx::kit::utils::toString(
            nx::sdk::Uuid::fromStdString(std::to_string(object.trackId))));

        // Set bounding box
        cv::Rect bbox = object.detection.bbox;
        objectMetadata->setBoundingBox(Rect(
            (float)bbox.x,
            (float)bbox.y,
            (float)bbox.width,
            (float)bbox.height
        ));

        // Set confidence
        objectMetadata->setConfidence(object.detection.confidence);

        // Add attributes
        objectMetadata->addAttribute(makePtr<Attribute>(
            Attribute::kSpeedAttributeName,
            std::to_string(object.speed)
        ));

        if (object.isStationary)
        {
            objectMetadata->addAttribute(makePtr<Attribute>(
                "stationary",
                "true"
            ));
        }

        // Add to packet
        metadataPacket->addItem(objectMetadata.get());
    }

    pushMetadataPacket(metadataPacket.releasePtr());
}

void DeviceAgent::generateEventMetadata(
    const std::vector<AnalyticsEvent>& events,
    int64_t timestampUs)
{
    for (const auto& event : events)
    {
        auto eventPacket = makePtr<EventMetadataPacket>();
        eventPacket->setTimestampUs(event.timestampUs);
        eventPacket->setDurationUs(event.durationUs);

        auto eventMetadata = makePtr<EventMetadata>();
        eventMetadata->setTypeId(event.eventType);
        eventMetadata->setCaption(event.caption);
        eventMetadata->setDescription(event.description);

        // Set bounding box if available
        if (event.boundingBox.area() > 0)
        {
            eventMetadata->setBoundingBox(Rect(
                (float)event.boundingBox.x,
                (float)event.boundingBox.y,
                (float)event.boundingBox.width,
                (float)event.boundingBox.height
            ));
        }

        // Add track ID if available
        if (event.trackId >= 0)
        {
            eventMetadata->setTrackId(nx::kit::utils::toString(
                nx::sdk::Uuid::fromStdString(std::to_string(event.trackId))));
        }

        // Add custom attributes
        for (const auto& attr : event.attributes)
        {
            eventMetadata->addAttribute(makePtr<Attribute>(
                attr.first,
                attr.second
            ));
        }

        eventPacket->addItem(eventMetadata.get());
        pushMetadataPacket(eventPacket.releasePtr());

        NX_PRINT << "Event: " << event.caption << " - " << event.description;
    }
}

void DeviceAgent::loadConfiguration()
{
    // Load configuration from settings or use defaults
    m_config = PluginConfiguration();

    // In a production plugin, you would load settings from the Engine's settings model
    // For now, we'll use the default configuration

    NX_PRINT << "Configuration loaded: model=" << m_config.modelType
             << " confidence=" << m_config.confidenceThreshold
             << " frameSkip=" << m_config.frameSkipRate;
}

void DeviceAgent::doSetNeededMetadataTypes(
    Result<void>* /*outValue*/,
    const IMetadataTypes* /*neededMetadataTypes*/)
{
    // Optional: React to changes in needed metadata types
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
