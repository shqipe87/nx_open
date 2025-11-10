// Copyright 2024. All Rights Reserved.

#include "rules_engine.h"

#include <nx/kit/debug.h>
#include <sstream>
#include <algorithm>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

RulesEngine::RulesEngine()
    : m_ruleDebounceUs(5000000)  // 5 seconds debounce
{
}

RulesEngine::~RulesEngine()
{
}

void RulesEngine::initialize(const PluginConfiguration& config)
{
    m_config = config;
    m_rules = config.rules;

    NX_PRINT << "Rules engine initialized with " << m_rules.size() << " rules";
}

std::vector<AnalyticsEvent> RulesEngine::evaluate(
    const std::vector<TrackedObject>& objects,
    int64_t timestampUs)
{
    std::vector<AnalyticsEvent> events;

    // Get current hour for time-based rules
    time_t timeSeconds = timestampUs / 1000000;
    struct tm* timeInfo = localtime(&timeSeconds);
    int currentHour = timeInfo->tm_hour;

    for (const auto& rule : m_rules)
    {
        if (!rule.enabled)
            continue;

        for (const auto& object : objects)
        {
            // Build rule context
            RuleContext context;
            context.object = &object;
            context.timestampUs = timestampUs;
            context.currentHour = currentHour;
            context.allObjects = &objects;

            // Count objects in zones if rule specifies a zone
            if (!rule.zone.empty())
            {
                context.objectCountInZone = countObjectsInZone(objects, rule.zone);
            }
            else
            {
                context.objectCountInZone = 0;
            }

            // Evaluate rule
            bool ruleMatched = evaluateRule(rule, context);

            if (ruleMatched)
            {
                // Check debounce
                auto& lastFired = m_ruleLastFiredTime[rule.name][object.trackId];
                if (timestampUs - lastFired < m_ruleDebounceUs)
                {
                    continue;  // Skip if fired recently
                }

                // Create event
                AnalyticsEvent event = createRuleEvent(rule, context);
                events.push_back(event);

                // Update fired state
                m_ruleStates[rule.name][object.trackId] = true;
                m_ruleLastFiredTime[rule.name][object.trackId] = timestampUs;

                NX_PRINT << "Rule matched: " << rule.name;
            }
            else
            {
                m_ruleStates[rule.name][object.trackId] = false;
            }
        }
    }

    return events;
}

bool RulesEngine::evaluateRule(const BusinessRule& rule, const RuleContext& context)
{
    // Check time constraints
    if (context.currentHour < rule.startHour || context.currentHour >= rule.endHour)
    {
        return false;
    }

    // If condition string is provided, evaluate it
    if (!rule.condition.empty())
    {
        return evaluateCondition(rule.condition, context);
    }

    // Otherwise, use simple rule parameters
    bool matches = true;

    // Check object type
    if (!rule.objectType.empty())
    {
        if (context.object->detection.className != rule.objectType)
        {
            return false;
        }
    }

    // Check zone
    if (!rule.zone.empty())
    {
        bool inZone = false;
        for (const auto& zoneName : context.object->currentZones)
        {
            if (zoneName == rule.zone)
            {
                inZone = true;
                break;
            }
        }
        if (!inZone)
            return false;
    }

    // Check count constraints
    if (rule.minCount >= 0 && context.objectCountInZone < rule.minCount)
    {
        return false;
    }
    if (rule.maxCount >= 0 && context.objectCountInZone > rule.maxCount)
    {
        return false;
    }

    // Check speed
    if (rule.minSpeed >= 0 && context.object->speed < rule.minSpeed)
    {
        return false;
    }

    return matches;
}

bool RulesEngine::evaluateCondition(const std::string& condition, const RuleContext& context)
{
    // Simple expression parser supporting AND (&&) and OR (||)

    // Split by OR first
    std::vector<std::string> orClauses;
    size_t pos = 0;
    size_t found = condition.find("||", pos);

    while (found != std::string::npos)
    {
        orClauses.push_back(condition.substr(pos, found - pos));
        pos = found + 2;
        found = condition.find("||", pos);
    }
    orClauses.push_back(condition.substr(pos));

    // Evaluate OR clauses
    for (const auto& orClause : orClauses)
    {
        // Split by AND
        std::vector<std::string> andClauses;
        size_t andPos = 0;
        size_t andFound = orClause.find("&&", andPos);

        while (andFound != std::string::npos)
        {
            andClauses.push_back(orClause.substr(andPos, andFound - andPos));
            andPos = andFound + 2;
            andFound = orClause.find("&&", andPos);
        }
        andClauses.push_back(orClause.substr(andPos));

        // Evaluate AND clauses
        bool allTrue = true;
        for (const auto& andClause : andClauses)
        {
            if (!evaluateSimpleCondition(andClause, context))
            {
                allTrue = false;
                break;
            }
        }

        if (allTrue)
            return true;  // At least one OR clause is true
    }

    return false;  // No OR clause was true
}

bool RulesEngine::evaluateSimpleCondition(const std::string& condition, const RuleContext& context)
{
    // Trim whitespace
    std::string trimmed = condition;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

    // Parse simple conditions like "object.type == 'person'" or "time.hour >= 18"

    // Find operator
    std::string op;
    size_t opPos = std::string::npos;

    if ((opPos = trimmed.find("==")) != std::string::npos)
        op = "==";
    else if ((opPos = trimmed.find("!=")) != std::string::npos)
        op = "!=";
    else if ((opPos = trimmed.find(">=")) != std::string::npos)
        op = ">=";
    else if ((opPos = trimmed.find("<=")) != std::string::npos)
        op = "<=";
    else if ((opPos = trimmed.find(">")) != std::string::npos)
        op = ">";
    else if ((opPos = trimmed.find("<")) != std::string::npos)
        op = "<";
    else
        return false;  // No valid operator

    std::string leftSide = trimmed.substr(0, opPos);
    std::string rightSide = trimmed.substr(opPos + op.length());

    // Trim
    leftSide.erase(0, leftSide.find_first_not_of(" \t"));
    leftSide.erase(leftSide.find_last_not_of(" \t") + 1);
    rightSide.erase(0, rightSide.find_first_not_of(" \t"));
    rightSide.erase(rightSide.find_last_not_of(" \t") + 1);

    // Remove quotes from right side if present
    if (rightSide.front() == '\'' && rightSide.back() == '\'')
    {
        rightSide = rightSide.substr(1, rightSide.length() - 2);
    }

    // Get value from context
    std::string leftValue = getContextValue(leftSide, context);

    // Perform comparison
    if (op == "==")
    {
        return leftValue == rightSide;
    }
    else if (op == "!=")
    {
        return leftValue != rightSide;
    }
    else
    {
        // Numeric comparison
        try
        {
            float leftNum = std::stof(leftValue);
            float rightNum = std::stof(rightSide);

            if (op == ">")
                return leftNum > rightNum;
            else if (op == "<")
                return leftNum < rightNum;
            else if (op == ">=")
                return leftNum >= rightNum;
            else if (op == "<=")
                return leftNum <= rightNum;
        }
        catch (...)
        {
            return false;
        }
    }

    return false;
}

std::string RulesEngine::getContextValue(const std::string& key, const RuleContext& context) const
{
    if (key == "object.type")
        return context.object->detection.className;
    else if (key == "object.speed")
        return std::to_string(context.object->speed);
    else if (key == "object.trackId")
        return std::to_string(context.object->trackId);
    else if (key == "time.hour")
        return std::to_string(context.currentHour);
    else if (key == "zone.count")
        return std::to_string(context.objectCountInZone);
    else if (key.find("zone") == 0)
    {
        // Check if object is in zone
        std::string zoneName = key.substr(5);  // Remove "zone."
        for (const auto& zone : context.object->currentZones)
        {
            if (zone == zoneName)
                return "true";
        }
        return "false";
    }

    return "";
}

AnalyticsEvent RulesEngine::createRuleEvent(
    const BusinessRule& rule,
    const RuleContext& context)
{
    AnalyticsEvent event;
    event.eventType = "nx.wave_opencv.ruleViolation";
    event.caption = "Rule: " + rule.name;
    event.description = "Business rule '" + rule.name + "' triggered for " +
                       context.object->detection.className +
                       " (ID: " + std::to_string(context.object->trackId) + ")";
    event.timestampUs = context.timestampUs;
    event.durationUs = 0;
    event.boundingBox = context.object->detection.bbox;
    event.trackId = context.object->trackId;

    // Map priority to severity
    if (rule.priority == "high")
        event.severity = "high";
    else if (rule.priority == "medium")
        event.severity = "medium";
    else
        event.severity = "low";

    event.attributes["rule_name"] = rule.name;
    event.attributes["object_type"] = context.object->detection.className;

    if (!rule.zone.empty())
    {
        event.attributes["zone"] = rule.zone;
    }

    return event;
}

int RulesEngine::countObjectsInZone(
    const std::vector<TrackedObject>& objects,
    const std::string& zoneName) const
{
    int count = 0;
    for (const auto& object : objects)
    {
        for (const auto& zone : object.currentZones)
        {
            if (zone == zoneName)
            {
                count++;
                break;
            }
        }
    }
    return count;
}

void RulesEngine::addRule(const BusinessRule& rule)
{
    // Remove existing rule with same name
    removeRule(rule.name);

    m_rules.push_back(rule);
    NX_PRINT << "Added rule: " << rule.name;
}

void RulesEngine::removeRule(const std::string& ruleName)
{
    auto it = std::remove_if(m_rules.begin(), m_rules.end(),
        [&ruleName](const BusinessRule& r) { return r.name == ruleName; });

    if (it != m_rules.end())
    {
        m_rules.erase(it, m_rules.end());
        m_ruleStates.erase(ruleName);
        m_ruleLastFiredTime.erase(ruleName);
        NX_PRINT << "Removed rule: " << ruleName;
    }
}

void RulesEngine::setRuleEnabled(const std::string& ruleName, bool enabled)
{
    for (auto& rule : m_rules)
    {
        if (rule.name == ruleName)
        {
            rule.enabled = enabled;
            NX_PRINT << "Rule " << ruleName << " " << (enabled ? "enabled" : "disabled");
            return;
        }
    }
}

std::vector<BusinessRule> RulesEngine::getRules() const
{
    return m_rules;
}

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
