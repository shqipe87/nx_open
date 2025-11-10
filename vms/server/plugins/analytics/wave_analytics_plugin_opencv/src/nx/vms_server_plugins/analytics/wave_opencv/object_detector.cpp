// Copyright 2024. All Rights Reserved.

#include "object_detector.h"

#include <nx/kit/debug.h>
#include <algorithm>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

ObjectDetector::ObjectDetector()
    : m_confidenceThreshold(0.5f)
    , m_nmsThreshold(0.4f)
    , m_inputSize(640, 640)
    , m_initialized(false)
    , m_useGPU(false)
    , m_scaleFactor(1.0 / 255.0)
    , m_mean(0, 0, 0)
    , m_swapRB(true)
{
}

ObjectDetector::~ObjectDetector()
{
}

bool ObjectDetector::initialize(const PluginConfiguration& config)
{
    m_modelType = config.modelType;
    m_confidenceThreshold = config.confidenceThreshold;
    m_nmsThreshold = config.nmsThreshold;
    m_useGPU = config.useGPU;

    try
    {
        if (m_modelType.find("yolo") != std::string::npos)
        {
            if (!loadYOLOModel(config.modelPath))
            {
                NX_PRINT << "Failed to load YOLO model from: " << config.modelPath;
                return false;
            }
        }
        else if (m_modelType.find("mobilenet") != std::string::npos)
        {
            if (!loadMobileNetModel(config.modelPath))
            {
                NX_PRINT << "Failed to load MobileNet model from: " << config.modelPath;
                return false;
            }
        }
        else
        {
            NX_PRINT << "Unsupported model type: " << m_modelType;
            return false;
        }

        m_initialized = true;
        NX_PRINT << "Object detector initialized successfully with model: " << m_modelType;
        return true;
    }
    catch (const std::exception& e)
    {
        NX_PRINT << "Exception during detector initialization: " << e.what();
        return false;
    }
}

bool ObjectDetector::loadYOLOModel(const std::string& modelPath)
{
    try
    {
        // Load ONNX model
        m_net = cv::dnn::readNetFromONNX(modelPath);

        if (m_net.empty())
        {
            NX_PRINT << "Failed to load YOLO ONNX model";
            return false;
        }

        // Set backend and target
        if (m_useGPU && cv::cuda::getCudaEnabledDeviceCount() > 0)
        {
            NX_PRINT << "Using CUDA backend for inference";
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        }
        else
        {
            NX_PRINT << "Using CPU backend for inference";
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }

        // Get output layer names
        m_outputLayerNames = m_net.getUnconnectedOutLayersNames();

        // YOLO-specific parameters
        m_inputSize = cv::Size(640, 640);
        m_scaleFactor = 1.0 / 255.0;
        m_mean = cv::Scalar(0, 0, 0);
        m_swapRB = true;

        return true;
    }
    catch (const std::exception& e)
    {
        NX_PRINT << "Exception loading YOLO model: " << e.what();
        return false;
    }
}

bool ObjectDetector::loadMobileNetModel(const std::string& modelPath)
{
    try
    {
        // Load MobileNet SSD model (can be Caffe or TensorFlow format)
        // Assuming Caffe format: need both .caffemodel and .prototxt
        std::string prototxtPath = modelPath;
        size_t dotPos = prototxtPath.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            prototxtPath = prototxtPath.substr(0, dotPos) + ".prototxt";
        }

        m_net = cv::dnn::readNetFromCaffe(prototxtPath, modelPath);

        if (m_net.empty())
        {
            NX_PRINT << "Failed to load MobileNet model";
            return false;
        }

        // Set backend and target
        if (m_useGPU && cv::cuda::getCudaEnabledDeviceCount() > 0)
        {
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        }
        else
        {
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }

        // MobileNet-specific parameters
        m_inputSize = cv::Size(300, 300);
        m_scaleFactor = 1.0 / 127.5;
        m_mean = cv::Scalar(127.5, 127.5, 127.5);
        m_swapRB = false;

        return true;
    }
    catch (const std::exception& e)
    {
        NX_PRINT << "Exception loading MobileNet model: " << e.what();
        return false;
    }
}

std::vector<Detection> ObjectDetector::detect(const cv::Mat& frame)
{
    if (!m_initialized || frame.empty())
    {
        return {};
    }

    try
    {
        // Preprocess frame
        cv::Mat blob = preprocessFrame(frame);

        // Run inference
        m_net.setInput(blob);
        std::vector<cv::Mat> outputs;
        m_net.forward(outputs, m_outputLayerNames);

        // Post-process based on model type
        std::vector<Detection> detections;
        if (m_modelType.find("yolo") != std::string::npos)
        {
            detections = postprocessYOLO(outputs, frame.size());
        }
        else if (m_modelType.find("mobilenet") != std::string::npos)
        {
            detections = postprocessMobileNet(outputs[0], frame.size());
        }

        // Apply NMS
        return applyNMS(detections);
    }
    catch (const std::exception& e)
    {
        NX_PRINT << "Exception during detection: " << e.what();
        return {};
    }
}

cv::Mat ObjectDetector::preprocessFrame(const cv::Mat& frame)
{
    cv::Mat blob = cv::dnn::blobFromImage(
        frame,
        m_scaleFactor,
        m_inputSize,
        m_mean,
        m_swapRB,
        false  // crop
    );

    return blob;
}

std::vector<Detection> ObjectDetector::postprocessYOLO(
    const std::vector<cv::Mat>& outputs,
    const cv::Size& frameSize)
{
    std::vector<Detection> detections;

    if (outputs.empty())
        return detections;

    // YOLOv8 output format: [1, 84, 8400] or [1, num_classes+4, num_predictions]
    // Need to transpose to [8400, 84]
    cv::Mat output = outputs[0];

    // Reshape if needed
    if (output.dims == 3)
    {
        int rows = output.size[2];  // 8400
        int cols = output.size[1];  // 84
        output = output.reshape(1, cols);  // Reshape to [84, 8400]
        cv::transpose(output, output);     // Transpose to [8400, 84]
    }

    float* data = (float*)output.data;
    int numDetections = output.rows;
    int numClasses = output.cols - 4;  // First 4 are bbox coordinates

    float scaleX = (float)frameSize.width / m_inputSize.width;
    float scaleY = (float)frameSize.height / m_inputSize.height;

    for (int i = 0; i < numDetections; ++i)
    {
        float* row = data + i * output.cols;

        // Get bbox coordinates (center_x, center_y, width, height)
        float cx = row[0] * scaleX;
        float cy = row[1] * scaleY;
        float w = row[2] * scaleX;
        float h = row[3] * scaleY;

        // Find class with maximum confidence
        float maxConf = 0.0f;
        int classId = -1;
        for (int c = 0; c < numClasses; ++c)
        {
            float conf = row[4 + c];
            if (conf > maxConf)
            {
                maxConf = conf;
                classId = c;
            }
        }

        // Filter by confidence threshold
        if (maxConf > m_confidenceThreshold && classId >= 0 && classId < (int)COCO_CLASSES.size())
        {
            Detection det;
            det.bbox = cv::Rect(
                std::max(0, (int)(cx - w / 2)),
                std::max(0, (int)(cy - h / 2)),
                std::min(frameSize.width, (int)w),
                std::min(frameSize.height, (int)h)
            );
            det.classId = classId;
            det.className = COCO_CLASSES[classId];
            det.confidence = maxConf;
            det.center = cv::Point2f(cx, cy);

            detections.push_back(det);
        }
    }

    return detections;
}

std::vector<Detection> ObjectDetector::postprocessMobileNet(
    const cv::Mat& output,
    const cv::Size& frameSize)
{
    std::vector<Detection> detections;

    // MobileNet SSD output: [1, 1, N, 7]
    // Each detection: [image_id, label, confidence, x_min, y_min, x_max, y_max]
    float* data = (float*)output.data;
    int numDetections = output.size[2];

    for (int i = 0; i < numDetections; ++i)
    {
        float* detection = data + i * 7;
        float confidence = detection[2];

        if (confidence > m_confidenceThreshold)
        {
            int classId = (int)detection[1];
            float xmin = detection[3] * frameSize.width;
            float ymin = detection[4] * frameSize.height;
            float xmax = detection[5] * frameSize.width;
            float ymax = detection[6] * frameSize.height;

            Detection det;
            det.bbox = cv::Rect(
                cv::Point(xmin, ymin),
                cv::Point(xmax, ymax)
            );
            det.classId = classId;
            det.className = classId < (int)COCO_CLASSES.size() ? COCO_CLASSES[classId] : "unknown";
            det.confidence = confidence;
            det.center = cv::Point2f((xmin + xmax) / 2, (ymin + ymax) / 2);

            detections.push_back(det);
        }
    }

    return detections;
}

std::vector<Detection> ObjectDetector::applyNMS(const std::vector<Detection>& detections)
{
    if (detections.empty())
        return detections;

    // Group detections by class
    std::map<int, std::vector<Detection>> classwiseDetections;
    for (const auto& det : detections)
    {
        classwiseDetections[det.classId].push_back(det);
    }

    std::vector<Detection> result;

    // Apply NMS per class
    for (auto& pair : classwiseDetections)
    {
        std::vector<Detection>& dets = pair.second;

        // Prepare for OpenCV NMSBoxes
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        for (const auto& det : dets)
        {
            boxes.push_back(det.bbox);
            scores.push_back(det.confidence);
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, scores, m_confidenceThreshold, m_nmsThreshold, indices);

        for (int idx : indices)
        {
            result.push_back(dets[idx]);
        }
    }

    return result;
}

std::vector<std::string> ObjectDetector::getSupportedModels()
{
    return {"yolov8n", "yolov8s", "yolov8m", "yolov8l", "yolov8x", "mobilenet_ssd"};
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
