// Copyright 2024. All Rights Reserved.
//
// Object detection using YOLO/MobileNet with OpenCV DNN

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>
#include <memory>

#include "types.h"

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Object detector using YOLO or MobileNet SSD models.
 * Supports ONNX, TensorFlow, and Caffe formats.
 */
class ObjectDetector
{
public:
    ObjectDetector();
    ~ObjectDetector();

    /**
     * Initialize detector with model and configuration.
     */
    bool initialize(const PluginConfiguration& config);

    /**
     * Detect objects in a frame.
     */
    std::vector<Detection> detect(const cv::Mat& frame);

    /**
     * Check if detector is initialized.
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * Get supported model types.
     */
    static std::vector<std::string> getSupportedModels();

private:
    /**
     * Load YOLO model (ONNX format).
     */
    bool loadYOLOModel(const std::string& modelPath);

    /**
     * Load MobileNet SSD model.
     */
    bool loadMobileNetModel(const std::string& modelPath);

    /**
     * Preprocess frame for inference.
     */
    cv::Mat preprocessFrame(const cv::Mat& frame);

    /**
     * Post-process YOLO outputs.
     */
    std::vector<Detection> postprocessYOLO(
        const std::vector<cv::Mat>& outputs,
        const cv::Size& frameSize);

    /**
     * Post-process MobileNet outputs.
     */
    std::vector<Detection> postprocessMobileNet(
        const cv::Mat& output,
        const cv::Size& frameSize);

    /**
     * Apply Non-Maximum Suppression.
     */
    std::vector<Detection> applyNMS(const std::vector<Detection>& detections);

private:
    cv::dnn::Net m_net;
    std::string m_modelType;
    float m_confidenceThreshold;
    float m_nmsThreshold;
    cv::Size m_inputSize;
    std::vector<std::string> m_outputLayerNames;
    bool m_initialized;
    bool m_useGPU;

    // Model-specific parameters
    float m_scaleFactor;
    cv::Scalar m_mean;
    bool m_swapRB;
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
