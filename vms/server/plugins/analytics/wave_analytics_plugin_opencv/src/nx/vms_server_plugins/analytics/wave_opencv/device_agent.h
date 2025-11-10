// Copyright 2024. All Rights Reserved.
//
// Wave Analytics Plugin with OpenCV
// DeviceAgent class for processing video streams

#pragma once

#include <nx/sdk/analytics/helpers/consuming_device_agent.h>
#include <nx/sdk/analytics/i_uncompressed_video_frame.h>
#include <nx/sdk/helpers/uuid_helper.h>

#include <opencv2/opencv.hpp>
#include <memory>
#include <map>
#include <vector>

#include "types.h"

// Forward declarations
namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

class ObjectDetector;
class ObjectTracker;
class QueueManager;
class RulesEngine;
class BehavioralAnalyzer;
class CrowdAnalyzer;

/**
 * DeviceAgent class implementing IDeviceAgent interface.
 * Processes video frames from a specific camera and generates metadata.
 */
class DeviceAgent: public nx::sdk::analytics::ConsumingDeviceAgent
{
public:
    DeviceAgent(const nx::sdk::IDeviceInfo* deviceInfo);
    virtual ~DeviceAgent() override;

protected:
    /**
     * Returns DeviceAgent-level manifest.
     */
    virtual std::string manifestString() const override;

    /**
     * Called when plugin is activated for the device.
     */
    virtual bool pushUncompressedVideoFrame(
        const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame) override;

    /**
     * Called when settings change.
     */
    virtual void doSetNeededMetadataTypes(
        nx::sdk::Result<void>* outValue,
        const nx::sdk::analytics::IMetadataTypes* neededMetadataTypes) override;

private:
    /**
     * Process a single video frame through the analytics pipeline.
     */
    void processVideoFrame(
        const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame);

    /**
     * Convert SDK video frame to OpenCV Mat.
     */
    cv::Mat convertFrameToMat(
        const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame);

    /**
     * Generate and push object detection metadata.
     */
    void generateObjectMetadata(
        const std::vector<TrackedObject>& objects,
        int64_t timestampUs);

    /**
     * Generate and push event metadata.
     */
    void generateEventMetadata(
        const std::vector<AnalyticsEvent>& events,
        int64_t timestampUs);

    /**
     * Load configuration from settings.
     */
    void loadConfiguration();

private:
    nx::sdk::Uuid m_deviceId;

    // Core analytics components
    std::unique_ptr<ObjectDetector> m_detector;
    std::unique_ptr<ObjectTracker> m_tracker;
    std::unique_ptr<QueueManager> m_queueManager;
    std::unique_ptr<RulesEngine> m_rulesEngine;
    std::unique_ptr<BehavioralAnalyzer> m_behavioralAnalyzer;
    std::unique_ptr<CrowdAnalyzer> m_crowdAnalyzer;

    // Frame processing state
    int64_t m_frameNumber = 0;
    int64_t m_lastProcessedTimestampUs = 0;
    bool m_initialized = false;

    // Configuration
    PluginConfiguration m_config;

    // Performance tracking
    int m_frameSkipCounter = 0;
    int m_frameSkipRate = 1; // Process every Nth frame
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
