# 24 - 监控运维手册与应急响应 (Monitoring Playbooks & Incident Response)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Google SRE Workbook](https://sre.google/workbook/table-of-contents/)

## 概述

本文档提供生产环境监控系统的标准化运维手册、常见故障应急响应流程和SOP操作指南，帮助运维团队快速定位问题、标准化处理流程，提升故障响应效率和系统稳定性。

---

## 一、监控系统日常运维手册

### 1.1 日常巡检清单

#### 每日必检项目
```yaml
daily_inspection_checklist:
  system_health_check:
    prometheus_status:
      - server_uptime: "> 24h"
      - scrape_success_rate: "> 99%"
      - storage_space_usage: "< 80%"
      - query_success_rate: "> 99%"
      
    alertmanager_status:
      - cluster_health: "all_nodes_healthy"
      - notification_delivery: "> 99% success"
      - silences_count: "< 10 active"
      - configuration_reload: "last_24h_successful"
      
    grafana_status:
      - dashboard_availability: "> 99.9%"
      - data_source_connectivity: "all_green"
      - user_session_count: "within_normal_range"
      - plugin_status: "all_active"
      
  data_quality_validation:
    metric_validation:
      - time_series_count: "expected_range_check"
      - sample_rate_consistency: "no_abnormal_spikes"
      - label_cardinality: "< 1000 per_metric"
      - staleness_detection: "no_expired_metrics"
      
    log_validation:
      - log_ingestion_rate: "consistent_with_baseline"
      - parsing_errors: "< 1% of_total"
      - storage_utilization: "< 85%"
      - retention_policy_compliance: "verified"
      
  performance_monitoring:
    resource_utilization:
      - cpu_usage: "< 70% average"
      - memory_usage: "< 80% average"
      - disk_io_wait: "< 10ms average"
      - network_throughput: "within_capacity"
      
    query_performance:
      - average_query_latency: "< 500ms"
      - slow_queries_count: "< 5 per_hour"
      - concurrent_connections: "within_limits"
      - cache_hit_ratio: "> 80%"
```

### 1.2 定期维护任务

#### 周期性维护操作
```yaml
scheduled_maintenance:
  weekly_tasks:
    cleanup_operations:
      - log_retention_cleanup: "remove_logs_older_than_retention"
      - temporary_file_removal: "clean_tmp_directories"
      - cache_invalidaton: "clear_expired_cache_entries"
      - zombie_process_kill: "terminate_orphaned_processes"
      
    configuration_review:
      - alert_rule_validation: "verify_no_false_positives"
      - dashboard_review: "update_outdated_visualizations"
      - data_source_health: "test_all_connections"
      - backup_verification: "restore_test_execution"
      
  monthly_tasks:
    system_optimization:
      - storage_compaction: "tsdb_block_compaction"
      - index_optimization: "rebuild_fragmented_indexes"
      - configuration_audit: "full_system_configuration_review"
      - capacity_planning: "analyze_growth_trends"
      
    security_maintenance:
      - certificate_renewal: "ssl_certificate_updates"
      - access_log_review: "security_audit_analysis"
      - vulnerability_scanning: "dependency_security_check"
      - permission_audit: "rbac_configuration_review"
      
  quarterly_tasks:
    major_upgrade:
      - version_upgrade_planning: "coordinate_with_stakeholders"
      - compatibility_testing: "pre_upgrade_validation"
      - rollback_preparation: "backup_and_recovery_test"
      - performance_benchmarking: "post_upgrade_validation"
```

---

## 二、常见故障应急响应

### 2.1 Prometheus故障处理

#### 核心故障场景及解决方案
```yaml
prometheus_incident_playbook:
  scrape_failure:
    symptoms:
      - target_down_alerts_firing
      - missing_metrics_in_dashboards
      - scrape_errors_in_logs
      
    diagnosis_steps:
      1. check_target_status: "kubectl get endpoints -n monitoring"
      2. verify_network_connectivity: "ping_and_telnet_tests"
      3. examine_scrape_configs: "prometheus_config_validation"
      4. analyze_target_logs: "application_log_investigation"
      
    resolution_actions:
      - network_issue_resolution: "firewall_or_dns_fix"
      - target_restart: "pod_or_service_restart"
      - configuration_update: "servicemonitor_adjustment"
      - temporary_scrape_disable: "selective_target_blacklisting"
      
  storage_full:
    symptoms:
      - disk_space_alert_firing
      - write_operation_failures
      - query_performance_degradation
      
    diagnosis_steps:
      1. check_disk_utilization: "df_and_du_commands"
      2. analyze_retention_policy: "current_vs_configured_retention"
      3. identify_large_metrics: "high_cardinality_analysis"
      4. review_compaction_status: "tsdb_compaction_progress"
      
    resolution_actions:
      - immediate_space_reclamation: "delete_old_data_or_extend_storage"
      - retention_policy_adjustment: "shorten_retention_or_increase_limits"
      - cardinality_reduction: "label_simplification_or_filtering"
      - storage_expansion: "add_more_disk_space_or_scale_out"
      
  query_performance_issues:
    symptoms:
      - slow_dashboard_loading
      - timeout_errors_in_ui
      - high_cpu_memory_usage
      
    diagnosis_steps:
      1. query_analysis: "slow_query_identification"
      2. resource_monitoring: "cpu_memory_disk_io_analysis"
      3. cache_effectiveness: "hit_rate_and_eviction_analysis"
      4. concurrent_load: "request_rate_and_parallelism_check"
      
    resolution_actions:
      - query_optimization: "rewrite_inefficient_queries"
      - resource_scaling: "increase_cpu_memory_allocation"
      - caching_enhancement: "adjust_cache_sizes_or_strategies"
      - load_balancing: "implement_query_distribution"
```

### 2.2 Alertmanager故障处理

#### 告警系统故障响应
```yaml
alertmanager_incident_playbook:
  notification_failure:
    symptoms:
      - alerts_not_being_sent
      - notification_delivery_errors
      - webhook_timeout_failures
      
    diagnosis_steps:
      1. check_receiver_status: "notification_channel_connectivity"
      2. verify_configuration: "alertmanager_config_validation"
      3. examine_logs: "error_message_analysis"
      4. test_connectivity: "manual_notification_testing"
      
    resolution_actions:
      - receiver_configuration_fix: "smtp_webhook_endpoint_correction"
      - authentication_update: "credential_refresh_or_update"
      - network_connectivity_restore: "firewall_proxy_adjustment"
      - fallback_mechanism_activation: "alternative_notification_channels"
      
  alert_storm_handling:
    symptoms:
      - excessive_alert_volume
      - notification_overload
      - system_performance_impact
      
    diagnosis_steps:
      1. alert_volume_analysis: "count_and_categorize_alerts"
      2. root_cause_identification: "find_triggering_conditions"
      3. suppression_effectiveness: "evaluate_existing_inhibition_rules"
      4. grouping_analysis: "assess_notification_grouping"
      
    resolution_actions:
      - temporary_alert_suppression: "silence_non_critical_alerts"
      - rate_limiting_implementation: "throttle_notification_frequency"
      - root_cause_remediation: "fix_underlying_issues"
      - configuration_optimization: "improve_grouping_and_inhibition"
```

---

## 三、SOP标准化操作流程

### 3.1 监控系统部署SOP

#### 标准化部署流程
```yaml
deployment_sop:
  pre_deployment_checklist:
    environment_preparation:
      - kubernetes_cluster_ready: "version_and_capacity_verified"
      - network_connectivity_established: "dns_and_firewall_configured"
      - storage_provisioned: "persistent_volumes_available"
      - security_compliance_met: "rbac_and_network_policies_applied"
      
    configuration_preparation:
      - values_yaml_customized: "environment_specific_settings"
      - secrets_managed: "tls_certificates_and_credentials"
      - backup_plan_created: "disaster_recovery_prepared"
      - monitoring_setup: "external_monitoring_configured"
      
  deployment_execution:
    helm_installation:
      step_1: "helm_repo_add prometheus-community https://prometheus-community.github.io/helm-charts"
      step_2: "helm_dependency_update monitoring-stack"
      step_3: "helm_install monitoring-stack prometheus-community/kube-prometheus-stack -f values-production.yaml"
      step_4: "kubectl_apply_custom_resources additional-configs/"
      
    post_deployment_validation:
      - component_health_check: "all_pods_running_and_ready"
      - service_availability: "endpoints_accessible_and_responding"
      - data_flow_verification: "metrics_scraping_and_alerting_working"
      - integration_testing: "dashboards_and_alerts_functional"
      
  rollback_procedure:
    trigger_conditions:
      - deployment_failure: "pods_crashing_or_unavailable"
      - configuration_error: "misconfigured_resources"
      - performance_degradation: "significant_performance_drop"
      - security_issue: "vulnerability_or_compliance_breach"
      
    rollback_steps:
      step_1: "helm_rollback monitoring-stack previous_release_version"
      step_2: "verify_previous_state_restored component_health_check"
      step_3: "investigate_root_cause failure_analysis"
      step_4: "document_lessons_learned incident_report_creation"
```

### 3.2 监控配置变更SOP

#### 配置变更管理流程
```yaml
configuration_change_sop:
  change_request_process:
    proposal_submission:
      - change_description: "detailed_change_explanation"
      - impact_assessment: "affected_components_and_risks"
      - rollback_plan: "reversal_procedure_documentation"
      - testing_plan: "validation_approach_outline"
      
    approval_workflow:
      - technical_review: "architecture_and_security_validation"
      - business_approval: "stakeholder_sign_off"
      - scheduling_coordination: "maintenance_window_arrangement"
      - communication_plan: "stakeholder_notification_strategy"
      
  implementation_execution:
    pre_change_validation:
      - backup_creation: "complete_system_state_backup"
      - health_check: "current_system_stability_verification"
      - test_environment_validation: "change_testing_in_staging"
      - stakeholder_notification: "change_start_announcement"
      
    change_execution:
      - configuration_update: "apply_approved_changes"
      - progressive_rollout: "canary_deployment_if_applicable"
      - monitoring_intensification: "enhanced_observability_during_change"
      - real_time_validation: "continuous_correctness_verification"
      
    post_change_validation:
      - functionality_testing: "comprehensive_feature_verification"
      - performance_benchmarking: "performance_impact_assessment"
      - stability_monitoring: "extended_observation_period"
      - stakeholder_confirmation: "success_notification_and_sign_off"
```

---

## 四、应急联系人与沟通机制

### 4.1 值班体系与联系方式

#### 生产环境值班架构
```yaml
on_call_structure:
  tier_1_support:
    role: 初级值班工程师
    responsibilities:
      - first_line_incident_triage
      - basic_troubleshooting_execution
      - escalation_decision_making
      - routine_maintenance_execution
      
    contact_info:
      - phone_number: "primary_on_call_phone"
      - slack_channel: "#monitoring-oncall"
      - email: "oncall-tier1@company.com"
      - escalation_time: "30 minutes_no_response"
      
  tier_2_support:
    role: 高级SRE工程师
    responsibilities:
      - complex_incident_investigation
      - system_architecture_expertise
      - root_cause_analysis_leadership
      - post_incident_review_conduction
      
    contact_info:
      - phone_number: "secondary_on_call_phone"
      - slack_channel: "#sre-team"
      - email: "sre-team@company.com"
      - escalation_time: "60 minutes_no_response"
      
  tier_3_support:
    role: 架构师/技术负责人
    responsibilities:
      - strategic_incident_management
      - cross_team_coordination
      - business_impact_assessment
      - long_term_solution_design
      
    contact_info:
      - phone_number: "management_on_call_phone"
      - slack_channel: "#tech-leadership"
      - email: "tech-leadership@company.com"
      - escalation_time: "immediate_for_critical_incidents"
```

### 4.2 事件沟通模板

#### 标准化沟通格式
```yaml
incident_communication_templates:
  incident_declaration:
    subject: "INCIDENT DECLARED: [SEVERITY] - [SERVICE] - [BRIEF_DESCRIPTION]"
    body_template: |
      **INCIDENT DETAILS**
      - Incident ID: INC-[TIMESTAMP]
      - Severity Level: {Critical/High/Medium/Low}
      - Affected Services: {SERVICE_LIST}
      - Start Time: {YYYY-MM-DD HH:MM:SS UTC}
      - Detected By: {MONITORING_SYSTEM/USER_REPORT}
      
      **CURRENT STATUS**
      - Impact Assessment: {USER_IMPACT_DESCRIPTION}
      - Root Cause: {PRELIMINARY_ANALYSIS}
      - Mitigation Actions: {TAKEN_ACTIONS}
      
      **RESPONSE TEAM**
      - Incident Commander: {NAME}
      - Communications Lead: {NAME}
      - Technical Lead: {NAME}
      
      **NEXT UPDATES**
      - Next Status Update: {TIME_INTERVAL}
      - Communication Channel: {SLACK_CHANNEL/EMAIL}
      
  status_update:
    template: |
      **INCIDENT UPDATE - {INCIDENT_ID}**
      - Current Status: {ACTIVE/RESOLVED/UNDER_INVESTIGATION}
      - Time Since Start: {DURATION}
      - Progress Made: {ACHIEVEMENTS_SINCE_LAST_UPDATE}
      - Next Steps: {PLANNED_ACTIONS}
      - Expected Resolution: {ETA_IF_AVAILABLE}
      
  incident_resolution:
    template: |
      **INCIDENT RESOLVED - {INCIDENT_ID}**
      - Resolution Time: {YYYY-MM-DD HH:MM:SS UTC}
      - Total Duration: {HOURS_MINUTES}
      - Root Cause: {FINAL_ROOT_CAUSE}
      - Resolution Actions: {STEPS_TAKEN_TO_FIX}
      - Preventive Measures: {FUTURE_PREVENTION_PLANS}
```

---

## 五、监控质量评估体系

### 5.1 监控成熟度评估

#### 企业监控成熟度模型
```yaml
monitoring_maturity_assessment:
  level_1_basic:
    characteristics:
      - manual_monitoring_setup
      - reactive_alerting_only
      - limited_metric_coverage
      - basic_dashboard_creation
      
    assessment_criteria:
      - metric_coverage: "< 50% of_system_components"
      - alert_accuracy: "< 60% true_positives"
      - mean_time_to_detection: "> 2_hours"
      - manual_intervention_required: "> 80% of_tasks"
      
  level_2_standardized:
    characteristics:
      - automated_deployment
      - standardized_alerting
      - comprehensive_dashboards
      - basic_analytics_capabilities
      
    assessment_criteria:
      - metric_coverage: "50-80% of_system_components"
      - alert_accuracy: "60-80% true_positives"
      - mean_time_to_detection: "30_minutes_to_1_hour"
      - manual_intervention_required: "50-80% of_tasks"
      
  level_3_optimized:
    characteristics:
      - intelligent_alerting
      - predictive_analytics
      - automated_response
      - cost_optimization_focus
      
    assessment_criteria:
      - metric_coverage: "80-95% of_system_components"
      - alert_accuracy: "80-95% true_positives"
      - mean_time_to_detection: "5-30_minutes"
      - manual_intervention_required: "20-50% of_tasks"
      
  level_4_autonomous:
    characteristics:
      - autonomous_operations
      - ai_driven_insights
      - self_healing_capabilities
      - business_value_optimization
      
    assessment_criteria:
      - metric_coverage: "> 95% of_system_components"
      - alert_accuracy: "> 95% true_positives"
      - mean_time_to_detection: "< 5_minutes"
      - manual_intervention_required: "< 20% of_tasks"
```

### 5.2 关键性能指标(KPI)

#### 监控系统效能指标
```yaml
monitoring_kpis:
  system_reliability:
    availability_metrics:
      - overall_system_uptime: "> 99.9%"
      - component_availability: "> 99.5%_per_component"
      - data_collection_success_rate: "> 99.9%"
      - alert_delivery_success_rate: "> 99.5%"
      
    performance_metrics:
      - average_query_response_time: "< 500ms"
      - dashboard_load_time: "< 2_seconds"
      - data_ingestion_latency: "< 30_seconds"
      - alert_notification_delay: "< 60_seconds"
      
  operational_efficiency:
    incident_management:
      - mean_time_to_detection: "< 15_minutes"
      - mean_time_to_resolution: "< 2_hours"
      - incident_response_time: "< 5_minutes"
      - false_positive_rate: "< 5%"
      
    maintenance_efficiency:
      - planned_maintenance_success_rate: "> 95%"
      - unplanned_downtime: "< 0.1%_monthly"
      - configuration_change_success_rate: "> 98%"
      - backup_restore_success_rate: "100%"
      
  business_impact:
    value_delivery:
      - business_service_coverage: "> 90%"
      - customer_impact_reduction: "> 50%_reduction"
      - cost_optimization_savings: "> 20%_annual_savings"
      - innovation_enablement: "measurable_productivity_gains"
```

---

## 六、附录：实用工具与脚本

### 6.1 监控健康检查脚本

#### 自动化健康检查工具
```bash
#!/bin/bash
# monitoring-health-check.sh

set -euo pipefail

CLUSTER_NAME=${1:-production}
NAMESPACE=${2:-monitoring}
OUTPUT_FORMAT=${3:-table}

echo "=== Monitoring System Health Check ==="
echo "Cluster: $CLUSTER_NAME | Namespace: $NAMESPACE"
echo "Timestamp: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo

# Prometheus检查
check_prometheus() {
    echo "🔍 Checking Prometheus..."
    
    # 检查Pod状态
    kubectl get pods -n $NAMESPACE -l app=prometheus -o wide
    
    # 检查服务状态
    kubectl get svc -n $NAMESPACE prometheus-operated
    
    # 检查指标抓取状态
    PROM_ENDPOINT=$(kubectl get svc -n $NAMESPACE prometheus-operated -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
    curl -s "http://$PROM_ENDPOINT/-/healthy" | head -1
    
    # 检查TSDB状态
    curl -s "http://$PROM_ENDPOINT/api/v1/status/tsdb" | jq '.status'
    
    echo "✅ Prometheus check completed"
    echo
}

# Alertmanager检查
check_alertmanager() {
    echo "🔔 Checking Alertmanager..."
    
    # 检查集群状态
    kubectl get pods -n $NAMESPACE -l app=alertmanager
    
    # 检查配置状态
    ALERT_ENDPOINT=$(kubectl get svc -n $NAMESPACE alertmanager-operated -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
    curl -s "http://$ALERT_ENDPOINT/-/healthy" | head -1
    
    # 检查告警状态
    curl -s "http://$ALERT_ENDPOINT/api/v2/alerts" | jq 'length'
    
    echo "✅ Alertmanager check completed"
    echo
}

# Grafana检查
check_grafana() {
    echo "📊 Checking Grafana..."
    
    # 检查Pod状态
    kubectl get pods -n $NAMESPACE -l app=grafana
    
    # 检查服务状态
    GRAFANA_ENDPOINT=$(kubectl get svc -n $NAMESPACE grafana -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
    curl -s "http://$GRAFANA_ENDPOINT/api/health" | jq '.database'
    
    echo "✅ Grafana check completed"
    echo
}

# 执行所有检查
main() {
    case $OUTPUT_FORMAT in
        "json")
            echo '{"checks":['
            check_prometheus | jq -R -s '{"prometheus": .}'
            check_alertmanager | jq -R -s '{"alertmanager": .}'
            check_grafana | jq -R -s '{"grafana": .}'
            echo ']}'
            ;;
        *)
            check_prometheus
            check_alertmanager
            check_grafana
            echo "🎉 All monitoring components are healthy!"
            ;;
    esac
}

main "$@"
```

### 6.2 告警规则验证工具

#### 告警规则语法和逻辑验证
```python
#!/usr/bin/env python3
# alert-rule-validator.py

import yaml
import requests
import sys
from typing import Dict, List, Any

class AlertRuleValidator:
    def __init__(self, prometheus_url: str):
        self.prometheus_url = prometheus_url
        self.validation_results = []
    
    def validate_rule_syntax(self, rule_file: str) -> bool:
        """验证告警规则YAML语法"""
        try:
            with open(rule_file, 'r') as f:
                rules = yaml.safe_load(f)
            
            # 基本结构验证
            if 'groups' not in rules:
                self.validation_results.append({
                    'file': rule_file,
                    'error': 'Missing required field: groups'
                })
                return False
                
            for group in rules['groups']:
                if 'name' not in group:
                    self.validation_results.append({
                        'file': rule_file,
                        'error': 'Group missing name field'
                    })
                    return False
                    
                if 'rules' not in group:
                    self.validation_results.append({
                        'file': rule_file,
                        'error': 'Group missing rules field'
                    })
                    return False
                    
            return True
            
        except Exception as e:
            self.validation_results.append({
                'file': rule_file,
                'error': f'Syntax error: {str(e)}'
            })
            return False
    
    def validate_promql_expression(self, expression: str) -> Dict[str, Any]:
        """验证PromQL表达式的有效性"""
        try:
            # 使用Prometheus API验证表达式
            response = requests.get(
                f"{self.prometheus_url}/api/v1/query",
                params={'query': expression},
                timeout=10
            )
            
            if response.status_code == 200:
                return {'valid': True, 'message': 'Expression is valid'}
            else:
                return {
                    'valid': False, 
                    'message': f'Invalid expression: {response.text}'
                }
                
        except Exception as e:
            return {'valid': False, 'message': f'Validation failed: {str(e)}'}
    
    def validate_alert_structure(self, rule: Dict[str, Any]) -> List[str]:
        """验证告警规则结构"""
        errors = []
        
        required_fields = ['alert', 'expr']
        for field in required_fields:
            if field not in rule:
                errors.append(f'Missing required field: {field}')
        
        # 验证标签
        if 'labels' in rule:
            if not isinstance(rule['labels'], dict):
                errors.append('Labels must be a dictionary')
        
        # 验证注解
        if 'annotations' in rule:
            if not isinstance(rule['annotations'], dict):
                errors.append('Annotations must be a dictionary')
                
        return errors
    
    def run_comprehensive_validation(self, rule_files: List[str]) -> Dict[str, Any]:
        """运行全面验证"""
        total_files = len(rule_files)
        passed_files = 0
        
        for rule_file in rule_files:
            print(f"Validating {rule_file}...")
            
            # 语法验证
            if not self.validate_rule_syntax(rule_file):
                continue
                
            # 结构验证
            with open(rule_file, 'r') as f:
                rules = yaml.safe_load(f)
            
            for group in rules['groups']:
                for rule in group.get('rules', []):
                    if 'alert' in rule:  # 是告警规则
                        errors = self.validate_alert_structure(rule)
                        if errors:
                            self.validation_results.extend([
                                {'file': rule_file, 'error': error} 
                                for error in errors
                            ])
                        else:
                            # PromQL验证
                            promql_result = self.validate_promql_expression(rule['expr'])
                            if not promql_result['valid']:
                                self.validation_results.append({
                                    'file': rule_file,
                                    'rule': rule.get('alert', 'unknown'),
                                    'error': promql_result['message']
                                })
                            else:
                                passed_files += 1
        
        return {
            'total_files': total_files,
            'passed_files': passed_files,
            'failed_files': total_files - passed_files,
            'validation_results': self.validation_results
        }

def main():
    if len(sys.argv) < 3:
        print("Usage: python alert-rule-validator.py <prometheus_url> <rule_files...>")
        sys.exit(1)
    
    prometheus_url = sys.argv[1]
    rule_files = sys.argv[2:]
    
    validator = AlertRuleValidator(prometheus_url)
    results = validator.run_comprehensive_validation(rule_files)
    
    print("\n=== Validation Results ===")
    print(f"Total files: {results['total_files']}")
    print(f"Passed: {results['passed_files']}")
    print(f"Failed: {results['failed_files']}")
    
    if results['validation_results']:
        print("\nErrors found:")
        for result in results['validation_results']:
            print(f"  File: {result['file']}")
            if 'rule' in result:
                print(f"  Rule: {result['rule']}")
            print(f"  Error: {result['error']}")
            print()

if __name__ == "__main__":
    main()
```

---

**核心原则**: 标准化、自动化、可重复的运维实践，确保监控系统的稳定可靠运行

---

**实施建议**: 建立完善的SOP体系，定期演练和优化应急响应流程，持续提升运维成熟度

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)