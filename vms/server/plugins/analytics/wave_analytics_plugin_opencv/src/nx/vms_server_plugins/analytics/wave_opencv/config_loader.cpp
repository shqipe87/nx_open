// Copyright 2024. All Rights Reserved.

#include "config_loader.h"

#include <nx/kit/debug.h>
#include <nx/kit/json.h>
#include <sstream>
#include <algorithm>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

ConfigurationLoader::ConfigurationLoader()
{
}

ConfigurationLoader::~ConfigurationLoader()
{
}

PluginConfiguration ConfigurationLoader::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        NX_PRINT << "Warning: Could not open config file: " << filePath;
        return PluginConfiguration();  // Return default config
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return loadFromString(buffer.str());
}

PluginConfiguration ConfigurationLoader::loadFromString(const std::string& jsonContent)
{
    PluginConfiguration config;

    try
    {
        // Parse detection settings
        config.modelType = getJsonValue(jsonContent, "\"model\"");
        if (config.modelType.empty())
            config.modelType = "yolov8n";

        config.modelPath = getJsonValue(jsonContent, "\"modelPath\"");
        if (config.modelPath.empty())
            config.modelPath = "/opt/wave_analytics/models/yolov8n.onnx";

        config.confidenceThreshold = getJsonFloat(jsonContent, "\"confidence\"", 0.5f);
        config.nmsThreshold = getJsonFloat(jsonContent, "\"nmsThreshold\"", 0.4f);
        config.useGPU = getJsonBool(jsonContent, "\"useGPU\"", true);

        // Parse tracking settings
        config.maxAge = getJsonInt(jsonContent, "\"maxAge\"", 30);
        config.minHits = getJsonInt(jsonContent, "\"minHits\"", 3);
        config.iouThreshold = getJsonFloat(jsonContent, "\"iouThreshold\"", 0.3f);

        // Parse analytics settings
        config.loiteringTimeSeconds = getJsonInt(jsonContent, "\"loiteringTimeSeconds\"", 30);
        config.enableFallDetection = getJsonBool(jsonContent, "\"enableFallDetection\"", true);
        config.enableAggressiveBehavior = getJsonBool(jsonContent, "\"enableAggressiveBehavior\"", true);
        config.enableQueueAnalytics = getJsonBool(jsonContent, "\"enableQueueAnalytics\"", true);
        config.enableCrowdDensity = getJsonBool(jsonContent, "\"enableCrowdDensity\"", true);

        // Parse crowd density thresholds
        config.crowdLowThreshold = getJsonInt(jsonContent, "\"low\"", 5);
        config.crowdMediumThreshold = getJsonInt(jsonContent, "\"medium\"", 10);
        config.crowdHighThreshold = getJsonInt(jsonContent, "\"high\"", 20);
        config.crowdCriticalThreshold = getJsonInt(jsonContent, "\"critical\"", 30);

        // Parse performance settings
        config.frameSkipRate = getJsonInt(jsonContent, "\"frameSkipRate\"", 1);
        config.maxObjectsPerFrame = getJsonInt(jsonContent, "\"maxObjectsPerFrame\"", 100);
        config.enableVisualization = getJsonBool(jsonContent, "\"enableVisualization\"", true);

        // Parse zones
        config.zones = parseZones(jsonContent);

        // Parse trip lines
        config.tripLines = parseTripLines(jsonContent);

        // Parse rules
        config.rules = parseRules(jsonContent);

        NX_PRINT << "Configuration loaded successfully: "
                 << "model=" << config.modelType
                 << ", zones=" << config.zones.size()
                 << ", rules=" << config.rules.size();
    }
    catch (const std::exception& e)
    {
        NX_PRINT << "Error parsing configuration: " << e.what();
        return PluginConfiguration();  // Return default config on error
    }

    return config;
}

std::vector<Zone> ConfigurationLoader::parseZones(const std::string& json)
{
    std::vector<Zone> zones;

    // Find "zones" array
    size_t zonesPos = json.find("\"zones\"");
    if (zonesPos == std::string::npos)
        return zones;

    // Simple zone parsing (in production, use a proper JSON library)
    // This is a minimal implementation for demonstration

    // Look for zone objects between [ and ]
    size_t arrayStart = json.find('[', zonesPos);
    size_t arrayEnd = json.find(']', arrayStart);

    if (arrayStart == std::string::npos || arrayEnd == std::string::npos)
        return zones;

    std::string zonesArray = json.substr(arrayStart, arrayEnd - arrayStart);

    // Parse each zone (simplified - looks for "name" fields)
    size_t pos = 0;
    while ((pos = zonesArray.find("\"name\"", pos)) != std::string::npos)
    {
        Zone zone;

        // Extract zone block
        size_t blockStart = zonesArray.rfind('{', pos);
        size_t blockEnd = zonesArray.find('}', pos);

        if (blockStart == std::string::npos || blockEnd == std::string::npos)
            break;

        std::string zoneBlock = zonesArray.substr(blockStart, blockEnd - blockStart + 1);

        zone.name = getJsonValue(zoneBlock, "\"name\"");
        zone.type = getJsonValue(zoneBlock, "\"type\"");
        zone.loiteringThresholdSeconds = getJsonInt(zoneBlock, "\"loiteringThresholdSeconds\"", 30);
        zone.maxOccupancy = getJsonInt(zoneBlock, "\"maxOccupancy\"", -1);
        zone.startHour = getJsonInt(zoneBlock, "\"startHour\"", 0);
        zone.endHour = getJsonInt(zoneBlock, "\"endHour\"", 24);
        zone.activeAtNight = getJsonBool(zoneBlock, "\"activeAtNight\"", true);

        // Parse coordinates (simplified)
        size_t coordsPos = zoneBlock.find("\"coordinates\"");
        if (coordsPos != std::string::npos)
        {
            // Example: [[0,0], [100,0], [100,100], [0,100]]
            // Simple parsing - extract numbers
            zone.polygon.clear();

            // This is a simplified parser - in production use proper JSON parsing
            size_t coordStart = zoneBlock.find('[', coordsPos);
            size_t coordEnd = zoneBlock.find(']', coordStart);

            if (coordStart != std::string::npos && coordEnd != std::string::npos)
            {
                std::string coordsStr = zoneBlock.substr(coordStart, coordEnd - coordStart);

                // Parse coordinate pairs
                std::vector<int> numbers;
                std::stringstream ss(coordsStr);
                std::string token;

                while (std::getline(ss, token, ','))
                {
                    // Remove non-numeric characters except minus
                    token.erase(std::remove_if(token.begin(), token.end(),
                        [](char c) { return !std::isdigit(c) && c != '-'; }), token.end());

                    if (!token.empty())
                    {
                        try {
                            numbers.push_back(std::stoi(token));
                        } catch (...) {}
                    }
                }

                // Create points from pairs
                for (size_t i = 0; i + 1 < numbers.size(); i += 2)
                {
                    zone.polygon.push_back(cv::Point(numbers[i], numbers[i + 1]));
                }
            }
        }

        // Calculate bounding box
        if (!zone.polygon.empty())
        {
            zone.boundingBox = cv::boundingRect(zone.polygon);
        }

        zones.push_back(zone);
        pos = blockEnd;
    }

    return zones;
}

std::vector<TripLine> ConfigurationLoader::parseTripLines(const std::string& json)
{
    std::vector<TripLine> lines;

    // Find "tripLines" array
    size_t linesPos = json.find("\"tripLines\"");
    if (linesPos == std::string::npos)
        return lines;

    size_t arrayStart = json.find('[', linesPos);
    size_t arrayEnd = json.find(']', arrayStart);

    if (arrayStart == std::string::npos || arrayEnd == std::string::npos)
        return lines;

    std::string linesArray = json.substr(arrayStart, arrayEnd - arrayStart);

    // Parse each line
    size_t pos = 0;
    while ((pos = linesArray.find("\"name\"", pos)) != std::string::npos)
    {
        TripLine line;

        size_t blockStart = linesArray.rfind('{', pos);
        size_t blockEnd = linesArray.find('}', pos);

        if (blockStart == std::string::npos || blockEnd == std::string::npos)
            break;

        std::string lineBlock = linesArray.substr(blockStart, blockEnd - blockStart + 1);

        line.name = getJsonValue(lineBlock, "\"name\"");
        line.directionAtoB = getJsonValue(lineBlock, "\"directionAtoB\"");
        line.directionBtoA = getJsonValue(lineBlock, "\"directionBtoA\"");
        line.bidirectional = getJsonBool(lineBlock, "\"bidirectional\"", false);

        // Parse start point
        size_t startPos = lineBlock.find("\"start\"");
        if (startPos != std::string::npos)
        {
            std::string startStr = lineBlock.substr(startPos + 8, 20);
            size_t comma = startStr.find(',');
            if (comma != std::string::npos)
            {
                try {
                    int x = std::stoi(startStr.substr(startStr.find('[') + 1, comma));
                    int y = std::stoi(startStr.substr(comma + 1, startStr.find(']')));
                    line.start = cv::Point(x, y);
                } catch (...) {}
            }
        }

        // Parse end point
        size_t endPos = lineBlock.find("\"end\"");
        if (endPos != std::string::npos)
        {
            std::string endStr = lineBlock.substr(endPos + 6, 20);
            size_t comma = endStr.find(',');
            if (comma != std::string::npos)
            {
                try {
                    int x = std::stoi(endStr.substr(endStr.find('[') + 1, comma));
                    int y = std::stoi(endStr.substr(comma + 1, endStr.find(']')));
                    line.end = cv::Point(x, y);
                } catch (...) {}
            }
        }

        lines.push_back(line);
        pos = blockEnd;
    }

    return lines;
}

std::vector<BusinessRule> ConfigurationLoader::parseRules(const std::string& json)
{
    std::vector<BusinessRule> rules;

    // Find "rules" array
    size_t rulesPos = json.find("\"rules\"");
    if (rulesPos == std::string::npos)
        return rules;

    size_t arrayStart = json.find('[', rulesPos);
    size_t arrayEnd = json.find(']', arrayStart);

    if (arrayStart == std::string::npos || arrayEnd == std::string::npos)
        return rules;

    std::string rulesArray = json.substr(arrayStart, arrayEnd - arrayStart);

    // Parse each rule
    size_t pos = 0;
    while ((pos = rulesArray.find("\"name\"", pos)) != std::string::npos)
    {
        BusinessRule rule;

        size_t blockStart = rulesArray.rfind('{', pos);
        size_t blockEnd = rulesArray.find('}', pos);

        if (blockStart == std::string::npos || blockEnd == std::string::npos)
            break;

        std::string ruleBlock = rulesArray.substr(blockStart, blockEnd - blockStart + 1);

        rule.name = getJsonValue(ruleBlock, "\"name\"");
        rule.condition = getJsonValue(ruleBlock, "\"condition\"");
        rule.action = getJsonValue(ruleBlock, "\"action\"");
        rule.priority = getJsonValue(ruleBlock, "\"priority\"");
        rule.objectType = getJsonValue(ruleBlock, "\"objectType\"");
        rule.zone = getJsonValue(ruleBlock, "\"zone\"");
        rule.minCount = getJsonInt(ruleBlock, "\"minCount\"", -1);
        rule.maxCount = getJsonInt(ruleBlock, "\"maxCount\"", -1);
        rule.minSpeed = getJsonInt(ruleBlock, "\"minSpeed\"", -1);
        rule.startHour = getJsonInt(ruleBlock, "\"startHour\"", 0);
        rule.endHour = getJsonInt(ruleBlock, "\"endHour\"", 24);
        rule.enabled = getJsonBool(ruleBlock, "\"enabled\"", true);

        rules.push_back(rule);
        pos = blockEnd;
    }

    return rules;
}

std::string ConfigurationLoader::getJsonValue(const std::string& json, const std::string& key)
{
    size_t pos = json.find(key);
    if (pos == std::string::npos)
        return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos)
        return "";

    pos = json.find('"', pos);
    if (pos == std::string::npos)
        return "";

    size_t endPos = json.find('"', pos + 1);
    if (endPos == std::string::npos)
        return "";

    return json.substr(pos + 1, endPos - pos - 1);
}

int ConfigurationLoader::getJsonInt(const std::string& json, const std::string& key, int defaultValue)
{
    size_t pos = json.find(key);
    if (pos == std::string::npos)
        return defaultValue;

    pos = json.find(':', pos);
    if (pos == std::string::npos)
        return defaultValue;

    // Skip whitespace
    while (pos < json.length() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t'))
        pos++;

    std::string numStr;
    while (pos < json.length() && (std::isdigit(json[pos]) || json[pos] == '-'))
        numStr += json[pos++];

    if (numStr.empty())
        return defaultValue;

    try {
        return std::stoi(numStr);
    } catch (...) {
        return defaultValue;
    }
}

float ConfigurationLoader::getJsonFloat(const std::string& json, const std::string& key, float defaultValue)
{
    size_t pos = json.find(key);
    if (pos == std::string::npos)
        return defaultValue;

    pos = json.find(':', pos);
    if (pos == std::string::npos)
        return defaultValue;

    while (pos < json.length() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t'))
        pos++;

    std::string numStr;
    while (pos < json.length() && (std::isdigit(json[pos]) || json[pos] == '.' || json[pos] == '-'))
        numStr += json[pos++];

    if (numStr.empty())
        return defaultValue;

    try {
        return std::stof(numStr);
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigurationLoader::getJsonBool(const std::string& json, const std::string& key, bool defaultValue)
{
    size_t pos = json.find(key);
    if (pos == std::string::npos)
        return defaultValue;

    pos = json.find(':', pos);
    if (pos == std::string::npos)
        return defaultValue;

    size_t truePos = json.find("true", pos);
    size_t falsePos = json.find("false", pos);
    size_t nextKeyPos = json.find('"', pos + 1);

    if (truePos != std::string::npos && truePos < nextKeyPos)
        return true;
    if (falsePos != std::string::npos && falsePos < nextKeyPos)
        return false;

    return defaultValue;
}

std::vector<std::string> ConfigurationLoader::getDefaultConfigPaths()
{
    return {
        "/etc/wave_analytics/config.json",
        "/opt/wave_analytics/config.json",
        "./config/config.json",
        "../config/config.json",
        "../../config/config.json"
    };
}

bool ConfigurationLoader::validate(const PluginConfiguration& config, std::string& errorMessage)
{
    // Validate model path exists
    std::ifstream modelFile(config.modelPath);
    if (!modelFile.good())
    {
        errorMessage = "Model file not found: " + config.modelPath;
        return false;
    }

    // Validate confidence threshold
    if (config.confidenceThreshold < 0.0f || config.confidenceThreshold > 1.0f)
    {
        errorMessage = "Confidence threshold must be between 0 and 1";
        return false;
    }

    // Validate NMS threshold
    if (config.nmsThreshold < 0.0f || config.nmsThreshold > 1.0f)
    {
        errorMessage = "NMS threshold must be between 0 and 1";
        return false;
    }

    // Validate frame skip rate
    if (config.frameSkipRate < 1 || config.frameSkipRate > 10)
    {
        errorMessage = "Frame skip rate must be between 1 and 10";
        return false;
    }

    return true;
}

std::string ConfigurationLoader::toJsonString(const PluginConfiguration& config)
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"detection\": {\n";
    json << "    \"model\": \"" << config.modelType << "\",\n";
    json << "    \"modelPath\": \"" << config.modelPath << "\",\n";
    json << "    \"confidence\": " << config.confidenceThreshold << ",\n";
    json << "    \"nmsThreshold\": " << config.nmsThreshold << ",\n";
    json << "    \"useGPU\": " << (config.useGPU ? "true" : "false") << "\n";
    json << "  },\n";
    json << "  \"analytics\": {\n";
    json << "    \"loiteringTimeSeconds\": " << config.loiteringTimeSeconds << ",\n";
    json << "    \"enableFallDetection\": " << (config.enableFallDetection ? "true" : "false") << ",\n";
    json << "    \"enableAggressiveBehavior\": " << (config.enableAggressiveBehavior ? "true" : "false") << ",\n";
    json << "    \"enableQueueAnalytics\": " << (config.enableQueueAnalytics ? "true" : "false") << ",\n";
    json << "    \"enableCrowdDensity\": " << (config.enableCrowdDensity ? "true" : "false") << "\n";
    json << "  },\n";
    json << "  \"performance\": {\n";
    json << "    \"frameSkipRate\": " << config.frameSkipRate << ",\n";
    json << "    \"enableVisualization\": " << (config.enableVisualization ? "true" : "false") << "\n";
    json << "  }\n";
    json << "}\n";
    return json.str();
}

bool ConfigurationLoader::saveToFile(const PluginConfiguration& config, const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        NX_PRINT << "Error: Could not write config file: " << filePath;
        return false;
    }

    file << toJsonString(config);
    file.close();
    return true;
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
