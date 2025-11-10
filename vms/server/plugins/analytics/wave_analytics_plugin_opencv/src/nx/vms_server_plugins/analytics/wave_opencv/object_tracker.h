// Copyright 2024. All Rights Reserved.
//
// Object tracking using SORT/DeepSORT algorithm

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <vector>
#include <map>
#include <memory>

#include "types.h"

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Kalman filter for tracking object motion.
 */
class KalmanBoxTracker
{
public:
    KalmanBoxTracker(const cv::Rect& initialBox);

    /**
     * Predict next state.
     */
    cv::Rect predict();

    /**
     * Update with new measurement.
     */
    void update(const cv::Rect& box);

    /**
     * Get current state.
     */
    cv::Rect getState() const;

    /**
     * Get velocity.
     */
    cv::Point2f getVelocity() const;

    /**
     * Get acceleration.
     */
    cv::Point2f getAcceleration() const;

private:
    cv::KalmanFilter m_kf;
    int m_timeSinceUpdate;
    int m_hits;
    int m_hitStreak;
    int m_age;
    cv::Point2f m_lastVelocity;
};

/**
 * Object tracker using SORT (Simple Online and Realtime Tracking) algorithm.
 * Can be extended to DeepSORT with appearance features.
 */
class ObjectTracker
{
public:
    ObjectTracker();
    ~ObjectTracker();

    /**
     * Initialize tracker with configuration.
     */
    void initialize(const PluginConfiguration& config);

    /**
     * Update tracks with new detections.
     * Returns list of tracked objects.
     */
    std::vector<TrackedObject> update(
        const std::vector<Detection>& detections,
        int64_t timestampUs);

    /**
     * Get all active tracks.
     */
    std::vector<TrackedObject> getActiveTracks() const;

    /**
     * Get track by ID.
     */
    TrackedObject* getTrack(int trackId);

    /**
     * Clear all tracks.
     */
    void clear();

private:
    /**
     * Associate detections to tracks using Hungarian algorithm.
     */
    void associateDetectionsToTracks(
        const std::vector<Detection>& detections,
        std::vector<int>& matchedDetections,
        std::vector<int>& matchedTracks,
        std::vector<int>& unmatchedDetections,
        std::vector<int>& unmatchedTracks);

    /**
     * Compute IOU cost matrix.
     */
    cv::Mat computeIOUMatrix(
        const std::vector<Detection>& detections,
        const std::vector<TrackedObject*>& tracks);

    /**
     * Hungarian algorithm for assignment.
     */
    void hungarianAlgorithm(
        const cv::Mat& costMatrix,
        std::vector<int>& assignment);

    /**
     * Create new track from detection.
     */
    void createNewTrack(const Detection& detection, int64_t timestampUs);

    /**
     * Update track with detection.
     */
    void updateTrack(TrackedObject& track, const Detection& detection, int64_t timestampUs);

    /**
     * Update motion features (velocity, acceleration, etc.).
     */
    void updateMotionFeatures(TrackedObject& track);

    /**
     * Mark track as lost.
     */
    void markTrackAsLost(TrackedObject& track);

    /**
     * Remove dead tracks.
     */
    void removeDeadTracks();

private:
    std::map<int, TrackedObject> m_tracks;
    std::map<int, std::unique_ptr<KalmanBoxTracker>> m_kalmanTrackers;
    int m_nextTrackId;
    int m_maxAge;
    int m_minHits;
    float m_iouThreshold;
    int64_t m_currentTimestampUs;
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
