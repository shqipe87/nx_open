// Copyright 2024. All Rights Reserved.

#include "object_tracker.h"

#include <nx/kit/debug.h>
#include <algorithm>
#include <limits>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

//-------------------------------------------------------------------------------------------------
// KalmanBoxTracker implementation

KalmanBoxTracker::KalmanBoxTracker(const cv::Rect& initialBox)
    : m_timeSinceUpdate(0)
    , m_hits(0)
    , m_hitStreak(0)
    , m_age(0)
    , m_lastVelocity(0, 0)
{
    // State: [cx, cy, w, h, vx, vy, vw, vh]
    // Measurement: [cx, cy, w, h]
    m_kf.init(8, 4, 0, CV_32F);

    // State transition matrix (constant velocity model)
    m_kf.transitionMatrix = (cv::Mat_<float>(8, 8) <<
        1, 0, 0, 0, 1, 0, 0, 0,  // cx = cx + vx
        0, 1, 0, 0, 0, 1, 0, 0,  // cy = cy + vy
        0, 0, 1, 0, 0, 0, 1, 0,  // w = w + vw
        0, 0, 0, 1, 0, 0, 0, 1,  // h = h + vh
        0, 0, 0, 0, 1, 0, 0, 0,  // vx = vx
        0, 0, 0, 0, 0, 1, 0, 0,  // vy = vy
        0, 0, 0, 0, 0, 0, 1, 0,  // vw = vw
        0, 0, 0, 0, 0, 0, 0, 1   // vh = vh
    );

    // Measurement matrix
    m_kf.measurementMatrix = cv::Mat::zeros(4, 8, CV_32F);
    m_kf.measurementMatrix.at<float>(0, 0) = 1;  // cx
    m_kf.measurementMatrix.at<float>(1, 1) = 1;  // cy
    m_kf.measurementMatrix.at<float>(2, 2) = 1;  // w
    m_kf.measurementMatrix.at<float>(3, 3) = 1;  // h

    // Process noise covariance
    cv::setIdentity(m_kf.processNoiseCov, cv::Scalar::all(1e-2));

    // Measurement noise covariance
    cv::setIdentity(m_kf.measurementNoiseCov, cv::Scalar::all(1e-1));

    // Error covariance
    cv::setIdentity(m_kf.errorCovPost, cv::Scalar::all(1));

    // Initialize state
    float cx = initialBox.x + initialBox.width / 2.0f;
    float cy = initialBox.y + initialBox.height / 2.0f;
    m_kf.statePost.at<float>(0) = cx;
    m_kf.statePost.at<float>(1) = cy;
    m_kf.statePost.at<float>(2) = initialBox.width;
    m_kf.statePost.at<float>(3) = initialBox.height;
}

cv::Rect KalmanBoxTracker::predict()
{
    cv::Mat prediction = m_kf.predict();
    m_age++;
    m_timeSinceUpdate++;
    m_hitStreak = 0;

    return getState();
}

void KalmanBoxTracker::update(const cv::Rect& box)
{
    m_timeSinceUpdate = 0;
    m_hits++;
    m_hitStreak++;

    // Create measurement
    cv::Mat measurement(4, 1, CV_32F);
    measurement.at<float>(0) = box.x + box.width / 2.0f;
    measurement.at<float>(1) = box.y + box.height / 2.0f;
    measurement.at<float>(2) = box.width;
    measurement.at<float>(3) = box.height;

    m_kf.correct(measurement);
}

cv::Rect KalmanBoxTracker::getState() const
{
    float cx = m_kf.statePost.at<float>(0);
    float cy = m_kf.statePost.at<float>(1);
    float w = m_kf.statePost.at<float>(2);
    float h = m_kf.statePost.at<float>(3);

    return cv::Rect(
        std::max(0, (int)(cx - w / 2)),
        std::max(0, (int)(cy - h / 2)),
        std::max(1, (int)w),
        std::max(1, (int)h)
    );
}

cv::Point2f KalmanBoxTracker::getVelocity() const
{
    return cv::Point2f(
        m_kf.statePost.at<float>(4),
        m_kf.statePost.at<float>(5)
    );
}

cv::Point2f KalmanBoxTracker::getAcceleration() const
{
    cv::Point2f currentVelocity = getVelocity();
    return cv::Point2f(
        currentVelocity.x - m_lastVelocity.x,
        currentVelocity.y - m_lastVelocity.y
    );
}

//-------------------------------------------------------------------------------------------------
// ObjectTracker implementation

ObjectTracker::ObjectTracker()
    : m_nextTrackId(1)
    , m_maxAge(30)
    , m_minHits(3)
    , m_iouThreshold(0.3f)
    , m_currentTimestampUs(0)
{
}

ObjectTracker::~ObjectTracker()
{
}

void ObjectTracker::initialize(const PluginConfiguration& config)
{
    m_maxAge = config.maxAge;
    m_minHits = config.minHits;
    m_iouThreshold = config.iouThreshold;

    NX_PRINT << "Object tracker initialized: maxAge=" << m_maxAge
             << " minHits=" << m_minHits
             << " iouThreshold=" << m_iouThreshold;
}

std::vector<TrackedObject> ObjectTracker::update(
    const std::vector<Detection>& detections,
    int64_t timestampUs)
{
    m_currentTimestampUs = timestampUs;

    // Predict new locations of existing tracks
    for (auto& pair : m_kalmanTrackers)
    {
        pair.second->predict();
    }

    // Associate detections to tracks
    std::vector<int> matchedDetections;
    std::vector<int> matchedTracks;
    std::vector<int> unmatchedDetections;
    std::vector<int> unmatchedTracks;

    associateDetectionsToTracks(
        detections,
        matchedDetections,
        matchedTracks,
        unmatchedDetections,
        unmatchedTracks
    );

    // Update matched tracks
    for (size_t i = 0; i < matchedDetections.size(); ++i)
    {
        int detIdx = matchedDetections[i];
        int trackId = matchedTracks[i];

        auto it = m_tracks.find(trackId);
        if (it != m_tracks.end())
        {
            updateTrack(it->second, detections[detIdx], timestampUs);
        }
    }

    // Mark unmatched tracks as lost
    for (int trackId : unmatchedTracks)
    {
        auto it = m_tracks.find(trackId);
        if (it != m_tracks.end())
        {
            markTrackAsLost(it->second);
        }
    }

    // Create new tracks for unmatched detections
    for (int detIdx : unmatchedDetections)
    {
        createNewTrack(detections[detIdx], timestampUs);
    }

    // Remove dead tracks
    removeDeadTracks();

    // Return active tracks
    return getActiveTracks();
}

void ObjectTracker::associateDetectionsToTracks(
    const std::vector<Detection>& detections,
    std::vector<int>& matchedDetections,
    std::vector<int>& matchedTracks,
    std::vector<int>& unmatchedDetections,
    std::vector<int>& unmatchedTracks)
{
    matchedDetections.clear();
    matchedTracks.clear();
    unmatchedDetections.clear();
    unmatchedTracks.clear();

    if (m_tracks.empty())
    {
        // All detections are unmatched
        for (size_t i = 0; i < detections.size(); ++i)
        {
            unmatchedDetections.push_back(i);
        }
        return;
    }

    if (detections.empty())
    {
        // All tracks are unmatched
        for (const auto& pair : m_tracks)
        {
            unmatchedTracks.push_back(pair.first);
        }
        return;
    }

    // Build list of track pointers
    std::vector<TrackedObject*> trackPtrs;
    std::vector<int> trackIds;
    for (auto& pair : m_tracks)
    {
        trackPtrs.push_back(&pair.second);
        trackIds.push_back(pair.first);
    }

    // Compute IOU cost matrix
    cv::Mat costMatrix = computeIOUMatrix(detections, trackPtrs);

    // Solve assignment problem using Hungarian algorithm
    std::vector<int> assignment;
    hungarianAlgorithm(costMatrix, assignment);

    // Process assignment results
    for (size_t i = 0; i < assignment.size(); ++i)
    {
        if (assignment[i] >= 0)
        {
            float iou = 1.0f - costMatrix.at<float>(i, assignment[i]);
            if (iou >= m_iouThreshold)
            {
                matchedDetections.push_back(i);
                matchedTracks.push_back(trackIds[assignment[i]]);
            }
            else
            {
                unmatchedDetections.push_back(i);
            }
        }
        else
        {
            unmatchedDetections.push_back(i);
        }
    }

    // Find unmatched tracks
    std::vector<bool> trackMatched(trackPtrs.size(), false);
    for (int trackIdx : matchedTracks)
    {
        for (size_t i = 0; i < trackIds.size(); ++i)
        {
            if (trackIds[i] == trackIdx)
            {
                trackMatched[i] = true;
                break;
            }
        }
    }

    for (size_t i = 0; i < trackMatched.size(); ++i)
    {
        if (!trackMatched[i])
        {
            unmatchedTracks.push_back(trackIds[i]);
        }
    }
}

cv::Mat ObjectTracker::computeIOUMatrix(
    const std::vector<Detection>& detections,
    const std::vector<TrackedObject*>& tracks)
{
    cv::Mat costMatrix = cv::Mat::zeros(detections.size(), tracks.size(), CV_32F);

    for (size_t i = 0; i < detections.size(); ++i)
    {
        for (size_t j = 0; j < tracks.size(); ++j)
        {
            float iou = calculateIOU(detections[i].bbox, tracks[j]->detection.bbox);
            costMatrix.at<float>(i, j) = 1.0f - iou;  // Convert to cost
        }
    }

    return costMatrix;
}

void ObjectTracker::hungarianAlgorithm(
    const cv::Mat& costMatrix,
    std::vector<int>& assignment)
{
    // Simplified assignment: greedy matching
    // For production, consider using a proper Hungarian algorithm implementation
    int numDetections = costMatrix.rows;
    int numTracks = costMatrix.cols;

    assignment.resize(numDetections, -1);
    std::vector<bool> trackAssigned(numTracks, false);

    for (int i = 0; i < numDetections; ++i)
    {
        float minCost = std::numeric_limits<float>::max();
        int bestTrack = -1;

        for (int j = 0; j < numTracks; ++j)
        {
            if (!trackAssigned[j])
            {
                float cost = costMatrix.at<float>(i, j);
                if (cost < minCost)
                {
                    minCost = cost;
                    bestTrack = j;
                }
            }
        }

        if (bestTrack >= 0 && minCost < (1.0f - m_iouThreshold))
        {
            assignment[i] = bestTrack;
            trackAssigned[bestTrack] = true;
        }
    }
}

void ObjectTracker::createNewTrack(const Detection& detection, int64_t timestampUs)
{
    TrackedObject track;
    track.trackId = m_nextTrackId++;
    track.detection = detection;
    track.trajectory.push_back(detection.center);
    track.firstSeenTimestampUs = timestampUs;
    track.lastSeenTimestampUs = timestampUs;
    track.framesSinceLastSeen = 0;
    track.isActive = true;
    track.velocity = cv::Point2f(0, 0);
    track.acceleration = cv::Point2f(0, 0);
    track.speed = 0;
    track.direction = 0;
    track.stationaryFrames = 0;
    track.isStationary = false;
    track.aspectRatio = (float)detection.bbox.width / detection.bbox.height;
    track.heightHistoryIndex = 0;
    for (int i = 0; i < 10; ++i)
        track.heightHistory[i] = detection.bbox.height;

    m_tracks[track.trackId] = track;
    m_kalmanTrackers[track.trackId] = std::make_unique<KalmanBoxTracker>(detection.bbox);

    NX_PRINT << "Created new track: " << track.trackId << " class: " << detection.className;
}

void ObjectTracker::updateTrack(TrackedObject& track, const Detection& detection, int64_t timestampUs)
{
    // Update Kalman filter
    auto it = m_kalmanTrackers.find(track.trackId);
    if (it != m_kalmanTrackers.end())
    {
        it->second->update(detection.bbox);
        track.detection.bbox = it->second->getState();
    }

    track.detection = detection;
    track.lastSeenTimestampUs = timestampUs;
    track.framesSinceLastSeen = 0;
    track.isActive = true;

    // Update trajectory
    track.trajectory.push_back(detection.center);
    if (track.trajectory.size() > 100)  // Keep last 100 points
    {
        track.trajectory.erase(track.trajectory.begin());
    }

    // Update height history for fall detection
    track.heightHistory[track.heightHistoryIndex] = detection.bbox.height;
    track.heightHistoryIndex = (track.heightHistoryIndex + 1) % 10;
    track.aspectRatio = (float)detection.bbox.width / detection.bbox.height;

    // Update motion features
    updateMotionFeatures(track);
}

void ObjectTracker::updateMotionFeatures(TrackedObject& track)
{
    if (track.trajectory.size() < 2)
        return;

    // Calculate velocity (pixels per frame)
    cv::Point2f current = track.trajectory.back();
    cv::Point2f previous = track.trajectory[track.trajectory.size() - 2];
    cv::Point2f newVelocity = current - previous;

    // Calculate acceleration
    track.acceleration = newVelocity - track.velocity;
    track.velocity = newVelocity;

    // Calculate speed and direction
    track.speed = std::sqrt(track.velocity.x * track.velocity.x + track.velocity.y * track.velocity.y);
    track.direction = std::atan2(track.velocity.y, track.velocity.x);

    // Update stationary status
    const float stationaryThreshold = 2.0f;  // pixels per frame
    if (track.speed < stationaryThreshold)
    {
        track.stationaryFrames++;
        track.isStationary = track.stationaryFrames > 10;  // 10 frames
    }
    else
    {
        track.stationaryFrames = 0;
        track.isStationary = false;
    }
}

void ObjectTracker::markTrackAsLost(TrackedObject& track)
{
    track.framesSinceLastSeen++;

    if (track.framesSinceLastSeen > m_maxAge)
    {
        track.isActive = false;
    }
}

void ObjectTracker::removeDeadTracks()
{
    auto it = m_tracks.begin();
    while (it != m_tracks.end())
    {
        if (!it->second.isActive)
        {
            m_kalmanTrackers.erase(it->first);
            it = m_tracks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::vector<TrackedObject> ObjectTracker::getActiveTracks() const
{
    std::vector<TrackedObject> activeTracks;
    for (const auto& pair : m_tracks)
    {
        if (pair.second.isActive && pair.second.framesSinceLastSeen == 0)
        {
            activeTracks.push_back(pair.second);
        }
    }
    return activeTracks;
}

TrackedObject* ObjectTracker::getTrack(int trackId)
{
    auto it = m_tracks.find(trackId);
    return (it != m_tracks.end()) ? &it->second : nullptr;
}

void ObjectTracker::clear()
{
    m_tracks.clear();
    m_kalmanTrackers.clear();
    m_nextTrackId = 1;
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
