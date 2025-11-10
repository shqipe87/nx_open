// Copyright 2024. All Rights Reserved.
//
// Business rules engine for custom analytics logic

#pragma once

#include <vector>
#include <map>
#include <string>
#include <functional>

#include "types.h"

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace wave_opencv {

/**
 * Rule evaluation context.
 */
struct RuleContext
{
    const TrackedObject* object;
    int64_t timestampUs;
    int currentHour;
    int objectCountInZone;
    const std::vector<TrackedObject>* allObjects;
};

/**
 * Business rules engine for evaluating custom rules.
 */
class RulesEngine
{
public:
    RulesEngine();
    ~RulesEngine();

    /**
     * Initialize with configuration.
     */
    void initialize(const PluginConfiguration& config);

    /**
     * Evaluate all rules for tracked objects.
     */
    std::vector<AnalyticsEvent> evaluate(
        const std::vector<TrackedObject>& objects,
        int64_t timestampUs);

    /**
     * Add or update a rule.
     */
    void addRule(const BusinessRule& rule);

    /**
     * Remove a rule by name.
     */
    void removeRule(const std::string& ruleName);

    /**
     * Enable/disable a rule.
     */
    void setRuleEnabled(const std::string& ruleName, bool enabled);

    /**
     * Get all rules.
     */
    std::vector<BusinessRule> getRules() const;

private:
    /**
     * Evaluate a single rule for an object.
     */
    bool evaluateRule(const BusinessRule& rule, const RuleContext& context);

    /**
     * Parse and evaluate condition expression.
     * Supports simple expressions like:
     * - "object.type == 'person'"
     * - "object.speed > 10"
     * - "zone == 'restricted'"
     * - "time.hour >= 18 && time.hour < 6"
     */
    bool evaluateCondition(const std::string& condition, const RuleContext& context);

    /**
     * Evaluate simple condition (no AND/OR).
     */
    bool evaluateSimpleCondition(const std::string& condition, const RuleContext& context);

    /**
     * Create event from rule match.
     */
    AnalyticsEvent createRuleEvent(
        const BusinessRule& rule,
        const RuleContext& context);

    /**
     * Count objects in a zone.
     */
    int countObjectsInZone(
        const std::vector<TrackedObject>& objects,
        const std::string& zoneName) const;

    /**
     * Get value from object/context.
     */
    std::string getContextValue(
        const std::string& key,
        const RuleContext& context) const;

private:
    PluginConfiguration m_config;
    std::vector<BusinessRule> m_rules;

    // Rule state tracking (to avoid duplicate events)
    std::map<std::string, std::map<int, bool>> m_ruleStates; // ruleName -> trackId -> fired
    std::map<std::string, std::map<int, int64_t>> m_ruleLastFiredTime;
    int64_t m_ruleDebounceUs; // Minimum time between same rule firings
};

} // namespace wave_opencv
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
