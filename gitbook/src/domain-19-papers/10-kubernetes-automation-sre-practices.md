# Kubernetes 自动化运维与SRE实践 (Automation and SRE Practices)

> **作者**: SRE专家 | **版本**: v2.2 | **更新时间**: 2026-02-07
> **适用场景**: 企业级运维自动化 | **复杂度**: ⭐⭐⭐⭐⭐

## 🎯 摘要

本文档深入探讨了Kubernetes环境下的自动化运维和SRE（站点可靠性工程）实践，基于大型互联网公司的SRE实践经验，提供从监控告警、故障响应到自动化运维的完整解决方案，帮助企业建立高效、可靠的运维体系。

## 1. SRE核心理念与原则

### 1.1 SRE基本原则

```yaml
SRE三大支柱:
  1. 可靠性工程 (Reliability Engineering)
     - SLI/SLO/Error Budget管理
     - 故障模式分析
     - 可靠性量化指标
  
  2. 自动化运维 (Automation)
     - 无人值守运维
     - 自愈能力
     - 故障自动恢复
  
  3. 速度与稳定性平衡 (Speed vs Stability)
     - 快速迭代与稳定运行
     - 错误预算管理
     - 风险控制策略
```

### 1.2 SLI/SLO/Error Budget管理

```yaml
核心SLI指标:
  可用性指标:
    - API可用性: 99.95% (p99.9)
    - 页面加载时间: < 2秒 (p95)
    - 错误率: < 0.05% (p99.9)
  
  性能指标:
    - API响应时间: < 100ms (p95)
    - 数据库查询时间: < 50ms (p95)
    - 系统吞吐量: > 10000 TPS
  
  功能指标:
    - 认证成功率: 99.99%
    - 支付成功率: 99.95%
    - 数据一致性: 99.99%
```

## 2. 监控告警体系建设

### 2.1 黄金指标监控

```yaml
RED方法 (Rate/Errors/Duration):
  Rate (速率):
    - HTTP请求速率: requests_per_second
    - 事务处理速率: transactions_per_second
    - 消息处理速率: messages_per_second
  
  Errors (错误):
    - HTTP错误率: error_rate
    - 业务错误率: business_error_rate
    - 系统错误率: system_error_rate
  
  Duration (持续时间):
    - 请求响应时间: response_time
    - 事务处理时间: transaction_time
    - 队列等待时间: queue_wait_time

USE方法 (Utilization/Saturation/Errors):
  Utilization (利用率):
    - CPU利用率: cpu_utilization
    - 内存利用率: memory_utilization
    - 磁盘利用率: disk_utilization
  
  Saturation (饱和度):
    - 队列长度: queue_length
    - 连接数: connection_count
    - 负载: system_load
  
  Errors (错误):
    - 硬件错误: hardware_errors
    - 系统错误: system_errors
    - 应用错误: application_errors
```

### 2.2 监控系统架构

```yaml
# 监控系统架构配置
apiVersion: v1
kind: Namespace
metadata:
  name: monitoring
---
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: main-prometheus
  namespace: monitoring
spec:
  serviceAccountName: prometheus
  serviceMonitorSelector: {}
  ruleSelector:
    matchLabels:
      role: alert-rules
  additionalScrapeConfigs:
    name: additional-scrape-configs
    key: prometheus-additional.yaml
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 500Gi
  resources:
    requests:
      memory: 4Gi
    limits:
      memory: 8Gi
  retention: 30d
  retentionSize: "100GB"
  walCompression: true
---
apiVersion: monitoring.coreos.com/v1
kind: Alertmanager
metadata:
  name: main-alertmanager
  namespace: monitoring
spec:
  replicas: 3
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 100Gi
  resources:
    requests:
      memory: 512Mi
    limits:
      memory: 1Gi
  configSecret: alertmanager-config
```

### 2.3 高级告警规则

```yaml
# 高级告警规则配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sre-alert-rules
  namespace: monitoring
spec:
  groups:
  - name: availability.rules
    rules:
    # API可用性告警
    - alert: APIAvailabilityLow
      expr: |
        1 - (sum(rate(http_requests_total{status=~"5..", handler!~"healthz|readyz"}[5m]))
        /
        sum(rate(http_requests_total{handler!~"healthz|readyz"}[5m]))) < 0.9995
      for: 5m
      labels:
        severity: critical
        category: availability
      annotations:
        summary: "API可用性低于SLA要求 (当前: {{ $value }}%)"
        description: "API可用性持续5分钟低于99.95% SLA要求"
    
    # 错误率告警
    - alert: HighErrorRate
      expr: |
        rate(http_requests_total{status=~"5.."}[5m])
        /
        rate(http_requests_total[5m]) > 0.01
      for: 2m
      labels:
        severity: warning
        category: errors
      annotations:
        summary: "错误率超过阈值 (当前: {{ $value }}%)"
        description: "HTTP错误率超过1%，可能影响用户体验"
    
    # 响应时间告警
    - alert: HighResponseTime
      expr: |
        histogram_quantile(0.95, 
        sum by(le) (rate(http_request_duration_seconds_bucket[5m])))
        > 1.0
      for: 3m
      labels:
        severity: warning
        category: performance
      annotations:
        summary: "API响应时间超过阈值 (p95: {{ $value }}s)"
        description: "API响应时间p95超过1秒，影响用户体验"
  
  - name: infrastructure.rules
    rules:
    # 节点资源告警
    - alert: NodeHighCPU
      expr: |
        100 - (avg by(instance) (
        rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
      for: 10m
      labels:
        severity: warning
        category: infrastructure
      annotations:
        summary: "节点CPU使用率过高 ({{ $labels.instance }}: {{ $value }}%)"
        description: "节点CPU使用率持续10分钟超过85%"
    
    # 内存压力告警
    - alert: NodeMemoryPressure
      expr: |
        (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100 < 15
      for: 5m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "节点内存压力过大 ({{ $labels.instance }}: {{ $value }}%)"
        description: "节点可用内存不足15%，可能导致OOM"
    
    # 磁盘空间告警
    - alert: DiskSpaceCritical
      expr: |
        (node_filesystem_avail_bytes / node_filesystem_size_bytes) * 100 < 10
      for: 10m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "磁盘空间不足 ({{ $labels.mountpoint }}: {{ $value }}%)"
        description: "磁盘可用空间不足10%，需要及时清理"
```

## 3. 自动化运维系统

### 3.1 运维机器人实现

```python
#!/usr/bin/env python3
# sre-automation-bot.py

import asyncio
import logging
from typing import Dict, List, Optional
from dataclasses import dataclass
from kubernetes import client, config
from kubernetes.stream import stream
import requests
import time
from datetime import datetime, timedelta

@dataclass
class Incident:
    id: str
    severity: str
    summary: str
    description: str
    timestamp: datetime
    affected_services: List[str]

class SREAutomationBot:
    def __init__(self):
        config.load_incluster_config()
        self.v1 = client.CoreV1Api()
        self.apps_v1 = client.AppsV1Api()
        self.logger = logging.getLogger(__name__)
        self.alert_manager_url = "http://alertmanager.monitoring.svc:9093"
        self.chat_webhook_url = "https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK"
    
    async def monitor_system_health(self):
        """监控系统健康状态"""
        while True:
            try:
                # 检查关键组件状态
                await self.check_kubernetes_components()
                
                # 检查节点状态
                await self.check_node_health()
                
                # 检查Pod状态
                await self.check_pod_health()
                
                # 检查存储状态
                await self.check_storage_health()
                
                await asyncio.sleep(30)  # 每30秒检查一次
                
            except Exception as e:
                self.logger.error(f"监控循环错误: {e}")
                await asyncio.sleep(60)
    
    async def check_kubernetes_components(self):
        """检查Kubernetes组件状态"""
        try:
            components = self.v1.list_component_status().items
            for component in components:
                if component.conditions[0].status != "True":
                    self.logger.warning(f"组件 {component.metadata.name} 异常: {component.conditions[0].message}")
                    
                    # 发送告警
                    incident = Incident(
                        id=f"component-{component.metadata.name}",
                        severity="critical",
                        summary=f"Kubernetes组件异常: {component.metadata.name}",
                        description=component.conditions[0].message,
                        timestamp=datetime.now(),
                        affected_services=["kubernetes-control-plane"]
                    )
                    await self.handle_incident(incident)
                    
        except Exception as e:
            self.logger.error(f"检查组件状态失败: {e}")
    
    async def check_node_health(self):
        """检查节点健康状态"""
        try:
            nodes = self.v1.list_node().items
            for node in nodes:
                for condition in node.status.conditions:
                    if condition.type == "Ready" and condition.status != "True":
                        self.logger.warning(f"节点 {node.metadata.name} 不健康: {condition.message}")
                        
                        # 尝试自动修复
                        await self.attempt_node_repair(node)
                        
        except Exception as e:
            self.logger.error(f"检查节点健康失败: {e}")
    
    async def check_pod_health(self):
        """检查Pod健康状态"""
        try:
            pods = self.v1.list_pod_for_all_namespaces().items
            for pod in pods:
                if pod.status.phase == "Failed":
                    self.logger.warning(f"Pod {pod.metadata.namespace}/{pod.metadata.name} 失败")
                    
                    # 发送告警
                    incident = Incident(
                        id=f"pod-{pod.metadata.namespace}-{pod.metadata.name}",
                        severity="high",
                        summary=f"Pod异常: {pod.metadata.namespace}/{pod.metadata.name}",
                        description=f"Pod状态: {pod.status.phase}, 原因: {pod.status.reason}",
                        timestamp=datetime.now(),
                        affected_services=[pod.metadata.labels.get('app', 'unknown')]
                    )
                    await self.handle_incident(incident)
                    
        except Exception as e:
            self.logger.error(f"检查Pod健康失败: {e}")
    
    async def attempt_node_repair(self, node):
        """尝试自动修复节点"""
        try:
            # 检查节点问题类型
            for condition in node.status.conditions:
                if condition.type == "DiskPressure":
                    # 清理节点上的垃圾容器
                    await self.cleanup_node_containers(node.metadata.name)
                    
                elif condition.type == "MemoryPressure":
                    # 驱逐部分Pod释放内存
                    await self.drain_node(node.metadata.name)
                    
                elif condition.type == "PIDPressure":
                    # 重启节点上的Pod
                    await self.restart_node_pods(node.metadata.name)
                    
        except Exception as e:
            self.logger.error(f"自动修复节点失败: {e}")
    
    async def cleanup_node_containers(self, node_name):
        """清理节点上的容器"""
        try:
            # 获取节点上的Pod列表
            pods = self.v1.list_pod_for_all_namespaces(
                field_selector=f"spec.nodeName={node_name}"
            ).items
            
            for pod in pods:
                if pod.status.phase in ["Succeeded", "Failed"]:
                    # 删除已完成的Pod
                    self.v1.delete_namespaced_pod(
                        pod.metadata.name,
                        pod.metadata.namespace
                    )
                    
        except Exception as e:
            self.logger.error(f"清理节点容器失败: {e}")
    
    async def drain_node(self, node_name):
        """驱逐节点上的Pod"""
        try:
            # 标记节点不可调度
            body = {
                "spec": {
                    "unschedulable": True
                }
            }
            self.v1.patch_node(node_name, body)
            
            # 驱逐节点上的Pod
            pods = self.v1.list_pod_for_all_namespaces(
                field_selector=f"spec.nodeName={node_name}"
            ).items
            
            for pod in pods:
                if pod.metadata.labels.get('critical', 'false') != 'true':
                    # 删除非关键Pod
                    self.v1.delete_namespaced_pod(
                        pod.metadata.name,
                        pod.metadata.namespace,
                        grace_period_seconds=30
                    )
                    
        except Exception as e:
            self.logger.error(f"驱逐节点Pod失败: {e}")
    
    async def restart_node_pods(self, node_name):
        """重启节点上的Pod"""
        try:
            pods = self.v1.list_pod_for_all_namespaces(
                field_selector=f"spec.nodeName={node_name}"
            ).items
            
            for pod in pods:
                # 优雅删除Pod，让其重新调度
                self.v1.delete_namespaced_pod(
                    pod.metadata.name,
                    pod.metadata.namespace,
                    grace_period_seconds=30
                )
                
        except Exception as e:
            self.logger.error(f"重启节点Pod失败: {e}")
    
    async def handle_incident(self, incident: Incident):
        """处理告警事件"""
        # 记录告警
        self.logger.info(f"处理告警: {incident.summary}")
        
        # 发送到告警系统
        await self.send_alert_to_monitoring(incident)
        
        # 发送到聊天系统
        await self.send_alert_to_chat(incident)
        
        # 根据严重程度执行自动化修复
        if incident.severity == "critical":
            await self.execute_emergency_procedures(incident)
        elif incident.severity == "high":
            await self.execute_high_priority_procedures(incident)
    
    async def send_alert_to_monitoring(self, incident: Incident):
        """发送告警到监控系统"""
        try:
            alert = {
                "labels": {
                    "alertname": incident.id,
                    "severity": incident.severity,
                    "summary": incident.summary
                },
                "annotations": {
                    "description": incident.description,
                    "timestamp": incident.timestamp.isoformat()
                }
            }
            
            response = requests.post(
                f"{self.alert_manager_url}/api/v2/alerts",
                json=[alert],
                timeout=10
            )
            
            if response.status_code != 200:
                self.logger.error(f"发送告警失败: {response.text}")
                
        except Exception as e:
            self.logger.error(f"发送告警到监控系统失败: {e}")
    
    async def send_alert_to_chat(self, incident: Incident):
        """发送告警到聊天系统"""
        try:
            message = {
                "text": f"🚨 {incident.severity.upper()} 告警: {incident.summary}",
                "attachments": [
                    {
                        "color": "danger" if incident.severity == "critical" else "warning",
                        "fields": [
                            {
                                "title": "描述",
                                "value": incident.description,
                                "short": False
                            },
                            {
                                "title": "时间",
                                "value": incident.timestamp.strftime("%Y-%m-%d %H:%M:%S"),
                                "short": True
                            },
                            {
                                "title": "受影响服务",
                                "value": ", ".join(incident.affected_services),
                                "short": True
                            }
                        ]
                    }
                ]
            }
            
            response = requests.post(
                self.chat_webhook_url,
                json=message,
                timeout=10
            )
            
            if response.status_code != 200:
                self.logger.error(f"发送告警到聊天系统失败: {response.text}")
                
        except Exception as e:
            self.logger.error(f"发送告警到聊天系统失败: {e}")
    
    async def execute_emergency_procedures(self, incident: Incident):
        """执行紧急处理程序"""
        try:
            self.logger.info(f"执行紧急处理程序: {incident.id}")
            
            # 1. 通知值班人员
            await self.notify_oncall_team(incident)
            
            # 2. 执行应急恢复
            await self.perform_emergency_recovery(incident)
            
            # 3. 启动备用系统
            await self.activate_backup_systems(incident)
            
        except Exception as e:
            self.logger.error(f"执行紧急处理程序失败: {e}")
    
    async def execute_high_priority_procedures(self, incident: Incident):
        """执行高优先级处理程序"""
        try:
            self.logger.info(f"执行高优先级处理程序: {incident.id}")
            
            # 1. 检查是否有自动修复方案
            await self.attempt_automated_fix(incident)
            
            # 2. 通知相关人员
            await self.notify_team_members(incident)
            
        except Exception as e:
            self.logger.error(f"执行高优先级处理程序失败: {e}")
    
    async def attempt_automated_fix(self, incident: Incident):
        """尝试自动化修复"""
        try:
            # 根据告警类型执行不同的修复策略
            if "pod" in incident.id and "Failed" in incident.description:
                # 重启失败的Pod
                parts = incident.id.split('-')
                if len(parts) >= 4:
                    namespace = parts[2]
                    pod_name = '-'.join(parts[3:])
                    
                    try:
                        self.v1.delete_namespaced_pod(pod_name, namespace)
                        self.logger.info(f"已删除失败的Pod: {namespace}/{pod_name}")
                    except:
                        pass
            
            elif "memory" in incident.summary.lower():
                # 重启相关Pod释放内存
                await self.restart_related_pods(incident)
                
        except Exception as e:
            self.logger.error(f"自动化修复失败: {e}")
    
    async def restart_related_pods(self, incident: Incident):
        """重启相关Pod"""
        try:
            for service in incident.affected_services:
                pods = self.v1.list_pod_for_all_namespaces(
                    label_selector=f"app={service}"
                ).items
                
                for pod in pods:
                    self.v1.delete_namespaced_pod(
                        pod.metadata.name,
                        pod.metadata.namespace
                    )
                    
        except Exception as e:
            self.logger.error(f"重启相关Pod失败: {e}")

async def main():
    bot = SREAutomationBot()
    await bot.monitor_system_health()

if __name__ == "__main__":
    asyncio.run(main())
```

### 3.2 自动化运维脚本

```bash
#!/bin/bash
# sre-automation-scripts.sh

# SRE自动化运维脚本集

# 1. 集群健康检查
check_cluster_health() {
    echo "=== 集群健康检查 ==="
    
    # 检查节点状态
    echo "1. 检查节点状态:"
    kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type,AGE:.metadata.creationTimestamp
    
    # 检查关键组件
    echo "2. 检查关键组件:"
    kubectl get componentstatuses
    
    # 检查系统Pod
    echo "3. 检查系统Pod状态:"
    kubectl get pods -n kube-system --field-selector=status.phase!=Running
    
    # 检查资源使用情况
    echo "4. 检查资源使用情况:"
    kubectl top nodes --sort-by=cpu
    kubectl top nodes --sort-by=memory
}

# 2. 自动扩容脚本
auto_scale_nodes() {
    echo "=== 自动节点扩容 ==="
    
    # 获取当前资源使用情况
    CPU_USAGE=$(kubectl top nodes --no-headers | awk '{sum+=$3} END {print sum/NR}' | sed 's/%//')
    MEMORY_USAGE=$(kubectl top nodes --no-headers | awk '{sum+=$5} END {print sum/NR}' | sed 's/%//')
    
    echo "当前CPU平均使用率: $CPU_USAGE%"
    echo "当前内存平均使用率: $MEMORY_USAGE%"
    
    # 检查是否需要扩容
    if [ "$CPU_USAGE" -gt 80 ] || [ "$MEMORY_USAGE" -gt 80 ]; then
        echo "资源使用率过高，执行扩容..."
        
        # 获取节点池名称
        NODE_POOL=$(kubectl get nodes -o jsonpath='{.items[0].metadata.labels.node\.kubernetes\.io/instance-type}')
        
        # 扩容节点池 (假设使用eksctl)
        # eksctl scale nodegroup --cluster=your-cluster --name=$NODE_POOL --nodes=10
    fi
}

# 3. 故障恢复脚本
recover_failed_deployments() {
    echo "=== 故障部署恢复 ==="
    
    # 查找失败的Deployment
    FAILED_DEPLOYMENTS=$(kubectl get deployments --all-namespaces -o json | \
        jq -r '.items[] | select(.status.conditions[]?.type=="Progressing" and .status.conditions[]?.status=="False") | "\(.metadata.namespace)/\(.metadata.name)"')
    
    if [ -z "$FAILED_DEPLOYMENTS" ]; then
        echo "没有发现失败的部署"
        return
    fi
    
    echo "发现失败的部署:"
    echo "$FAILED_DEPLOYMENTS"
    
    # 对每个失败的部署执行回滚
    for deployment in $FAILED_DEPLOYMENTS; do
        echo "正在回滚 $deployment..."
        kubectl rollout undo deployment/$deployment
        sleep 5
    done
}

# 4. 资源清理脚本
cleanup_resources() {
    echo "=== 资源清理 ==="
    
    # 清理已完成的Job
    COMPLETED_JOBS=$(kubectl get jobs --all-namespaces -o json | \
        jq -r '.items[] | select(.status.succeeded > 0) | "\(.metadata.namespace)/\(.metadata.name)"')
    
    if [ -n "$COMPLETED_JOBS" ]; then
        echo "清理已完成的Jobs:"
        for job in $COMPLETED_JOBS; do
            echo "删除 $job"
            kubectl delete job $job
        done
    fi
    
    # 清理失败的Pod
    FAILED_PODS=$(kubectl get pods --all-namespaces -o json | \
        jq -r '.items[] | select(.status.phase=="Failed") | "\(.metadata.namespace)/\(.metadata.name)"')
    
    if [ -n "$FAILED_PODS" ]; then
        echo "清理失败的Pods:"
        for pod in $FAILED_PODS; do
            echo "删除 $pod"
            kubectl delete pod $pod
        done
    fi
    
    # 清理未使用的ConfigMap
    kubectl get configmaps --all-namespaces --no-headers | \
        while read namespace name created; do
            if ! kubectl get pods -n $namespace -o yaml | grep -q $name; then
                echo "删除未使用的ConfigMap: $namespace/$name"
                kubectl delete configmap $name -n $namespace
            fi
        done
}

# 5. 性能优化脚本
optimize_performance() {
    echo "=== 性能优化 ==="
    
    # 检查资源请求和限制
    echo "检查资源请求/限制配置..."
    
    # 分析Pod资源使用情况
    kubectl top pods --all-namespaces --containers
    
    # 生成资源优化建议
    kubectl get pods --all-namespaces -o json | \
        jq -r '.items[] | select(.spec.containers[].resources.requests) | 
        "\(.metadata.namespace)/\(.metadata.name):\(.spec.containers[].name)"'
}

# 6. 备份脚本
backup_cluster_config() {
    echo "=== 集群配置备份 ==="
    
    BACKUP_DIR="/tmp/cluster-backup-$(date +%Y%m%d_%H%M%S)"
    mkdir -p $BACKUP_DIR
    
    # 备份命名空间
    kubectl get namespaces -o yaml > $BACKUP_DIR/namespaces.yaml
    
    # 备份存储类
    kubectl get storageclasses -o yaml > $BACKUP_DIR/storageclasses.yaml
    
    # 备份RBAC配置
    kubectl get clusterroles,clusterrolebindings,roles,rolebindings --all-namespaces -o yaml > $BACKUP_DIR/rbac.yaml
    
    # 备份关键配置
    kubectl get configmaps,secrets --all-namespaces -o yaml > $BACKUP_DIR/configs.yaml
    
    # 备份部署配置
    kubectl get deployments,statefulsets,daemonsets --all-namespaces -o yaml > $BACKUP_DIR/deployments.yaml
    
    echo "备份完成: $BACKUP_DIR"
}

# 主函数
main() {
    case "$1" in
        health-check)
            check_cluster_health
            ;;
        auto-scale)
            auto_scale_nodes
            ;;
        recover-failed)
            recover_failed_deployments
            ;;
        cleanup)
            cleanup_resources
            ;;
        optimize)
            optimize_performance
            ;;
        backup)
            backup_cluster_config
            ;;
        all)
            check_cluster_health
            auto_scale_nodes
            cleanup_resources
            ;;
        *)
            echo "用法: $0 {health-check|auto-scale|recover-failed|cleanup|optimize|backup|all}"
            exit 1
            ;;
    esac
}

main "$@"
```

## 4. 故障响应与恢复

### 4.1 故障响应流程

```yaml
# 故障响应流程配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: incident-response-playbook
  namespace: sre
data:
  critical-incident.yaml: |
    # 关键故障响应手册
    incident_types:
      api_outage:
        severity: critical
        response_time: 5m
        escalation_path:
          - level: 1
            role: oncall_engineer
            contact: pagerduty
            timeout: 15m
          - level: 2
            role: sre_lead
            contact: phone
            timeout: 30m
          - level: 3
            role: engineering_vp
            contact: phone
            timeout: 60m
        
        immediate_actions:
          - check_api_health
          - restart_api_pods
          - switch_to_backup_region
          - engage_database_team
        
        rollback_procedure:
          - rollback_recent_deployments
          - restore_from_backup
          - scale_down_traffic
        
        postmortem_requirements:
          - timeline_documentation
          - root_cause_analysis
          - action_items_definition
          - follow_up_meeting_schedule
      
      database_outage:
        severity: critical
        response_time: 3m
        escalation_path:
          - level: 1
            role: dba_oncall
            contact: pagerduty
            timeout: 10m
          - level: 2
            role: sre_engineer
            contact: phone
            timeout: 20m
          - level: 3
            role: database_architect
            contact: phone
            timeout: 40m
        
        immediate_actions:
          - check_database_connectivity
          - restart_database_pods
          - failover_to_replica
          - engage_infrastructure_team
        
        rollback_procedure:
          - restore_from_recent_backup
          - replay_transaction_logs
          - verify_data_consistency
```

### 4.2 自愈系统配置

```yaml
# 自愈系统配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: self-healing-config
  namespace: sre
data:
  healing-rules.yaml: |
    # 自愈规则配置
    healing_rules:
      pod_crash_loop:
        condition: "pod.status.phase == 'Failed' or pod.status.containerStatuses[*].restartCount > 5"
        action: "delete_pod_and_reschedule"
        cooldown: 300s
        max_attempts: 3
      
      node_unresponsive:
        condition: "node.status.conditions[Ready].status == 'False' for 5m"
        action: "cordon_and_drain_node"
        cooldown: 600s
        max_attempts: 1
      
      high_cpu_usage:
        condition: "node.cpu_usage > 85% for 10m"
        action: "scale_up_node_pool"
        cooldown: 300s
        max_attempts: 2
      
      high_memory_usage:
        condition: "pod.memory_usage > 90% for 5m"
        action: "restart_pod"
        cooldown: 120s
        max_attempts: 3
      
      service_unavailable:
        condition: "service.endpoint_count == 0 for 2m"
        action: "restart_deployment"
        cooldown: 300s
        max_attempts: 2
```

## 5. 变更管理与发布

### 5.1 变更管理流程

```yaml
# 变更管理配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: change-management-config
  namespace: sre
data:
  change-process.yaml: |
    # 变更管理流程
    change_categories:
      emergency:
        approval_level: engineering_director
        review_time: immediate
        deployment_window: anytime
        rollback_plan: mandatory
      
      standard:
        approval_level: team_lead
        review_time: 24h
        deployment_window: maintenance_window
        rollback_plan: required
      
      low_risk:
        approval_level: peer_review
        review_time: 2h
        deployment_window: business_hours
        rollback_plan: optional
    
    deployment_windows:
      maintenance:
        days: ["Saturday", "Sunday"]
        time: "02:00-06:00 UTC"
        duration: 4h
      
      business_hours:
        days: ["Monday-Friday"]
        time: "09:00-17:00 UTC"
        duration: 8h
    
    approval_workflow:
      - stage: code_review
        approvers: ["senior_engineer", "tech_lead"]
        criteria: ["all_tests_passed", "security_scan_passed"]
      
      - stage: qa_approval
        approvers: ["qa_lead"]
        criteria: ["feature_tested", "regression_tests_passed"]
      
      - stage: ops_approval
        approvers: ["sre_lead"]
        criteria: ["resource_impact_approved", "rollback_plan_verified"]
```

### 5.2 发布策略配置

```yaml
# 发布策略配置
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: progressive-rollout
  namespace: production
spec:
  replicas: 10
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: myapp
        image: myapp:v2.0.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            memory: "64Mi"
            cpu: "250m"
          limits:
            memory: "128Mi"
            cpu: "500m"
  strategy:
    blueGreen:
      activeService: myapp-active
      previewService: myapp-preview
      autoPromotionEnabled: false
      autoPromotionSeconds: 600
      scaleDownDelaySeconds: 30
      prePromotionAnalysis:
        templates:
        - templateName: success-rate
        args:
        - name: service-name
          value: myapp-preview
      postPromotionAnalysis:
        templates:
        - templateName: success-rate
        args:
        - name: service-name
          value: myapp-active
---
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  args:
  - name: service-name
  metrics:
  - name: success-rate
    interval: 30s
    count: 20
    successCondition: result[0] >= 0.95
    provider:
      prometheus:
        address: http://prometheus-server.monitoring.svc.cluster.local:9090
        query: |
          1 - (
            sum(rate(http_requests_total{service="{{args.service-name}}", code=~"5.."}[5m]))
          /
            sum(rate(http_requests_total{service="{{args.service-name}}"}[5m]))
          )
```

## 6. 容量规划与性能优化

### 6.1 容量规划工具

```python
#!/usr/bin/env python3
# capacity-planning.py

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures
import warnings
warnings.filterwarnings('ignore')

class CapacityPlanner:
    def __init__(self):
        self.data_history = []
        self.planning_horizon = 90  # 90天规划期
    
    def collect_resource_usage(self):
        """收集资源使用数据"""
        import subprocess
        import json
        
        # 收集节点资源使用
        cmd = "kubectl top nodes --no-headers"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        node_data = []
        for line in result.stdout.strip().split('\n'):
            if line:
                parts = line.split()
                if len(parts) >= 5:
                    node_data.append({
                        'timestamp': datetime.now(),
                        'node': parts[0],
                        'cpu_cores': int(parts[1].rstrip('m')),
                        'cpu_percent': int(parts[2].rstrip('%')),
                        'memory_bytes': self.parse_memory(parts[3]),
                        'memory_percent': int(parts[4].rstrip('%'))
                    })
        
        return node_data
    
    def parse_memory(self, mem_str):
        """解析内存字符串"""
        if mem_str.endswith('Ki'):
            return float(mem_str[:-2]) * 1024
        elif mem_str.endswith('Mi'):
            return float(mem_str[:-2]) * 1024 * 1024
        elif mem_str.endswith('Gi'):
            return float(mem_str[:-2]) * 1024 * 1024 * 1024
        else:
            return float(mem_str)
    
    def predict_resource_needs(self, historical_data, days_ahead=90):
        """预测未来资源需求"""
        # 转换为DataFrame
        df = pd.DataFrame(historical_data)
        df['timestamp'] = pd.to_datetime(df['timestamp'])
        df['days_since_start'] = (df['timestamp'] - df['timestamp'].min()).dt.days
        
        # 按天聚合数据
        daily_agg = df.groupby('days_since_start').agg({
            'cpu_percent': 'mean',
            'memory_percent': 'mean'
        }).reset_index()
        
        # 使用多项式回归进行预测
        X = daily_agg[['days_since_start']].values
        y_cpu = daily_agg['cpu_percent'].values
        y_memory = daily_agg['memory_percent'].values
        
        # CPU预测
        poly_features = PolynomialFeatures(degree=2)
        X_poly = poly_features.fit_transform(X)
        
        model_cpu = LinearRegression()
        model_cpu.fit(X_poly, y_cpu)
        
        model_memory = LinearRegression()
        model_memory.fit(X_poly, y_memory)
        
        # 预测未来数据
        future_days = np.arange(X.max() + 1, X.max() + days_ahead + 1).reshape(-1, 1)
        future_days_poly = poly_features.transform(future_days)
        
        future_cpu = model_cpu.predict(future_days_poly)
        future_memory = model_memory.predict(future_days_poly)
        
        return {
            'days': future_days.flatten(),
            'predicted_cpu': future_cpu,
            'predicted_memory': future_memory
        }
    
    def calculate_scaling_recommendations(self, predictions, current_capacity):
        """计算扩容建议"""
        cpu_threshold = 80  # CPU使用率阈值
        memory_threshold = 80  # 内存使用率阈值
        
        recommendations = []
        
        for i, day in enumerate(predictions['days']):
            cpu_needed = predictions['predicted_cpu'][i]
            memory_needed = predictions['predicted_memory'][i]
            
            if cpu_needed > cpu_threshold or memory_needed > memory_threshold:
                # 计算需要的容量增长
                cpu_growth = max(0, (cpu_needed - cpu_threshold) / cpu_threshold)
                memory_growth = max(0, (memory_needed - memory_threshold) / memory_threshold)
                
                growth_factor = max(cpu_growth, memory_growth) * 1.2  # 20%缓冲
                
                recommendations.append({
                    'day': day,
                    'date': datetime.now() + timedelta(days=int(day)),
                    'growth_factor': growth_factor,
                    'reason': f"CPU: {cpu_needed:.1f}%, Memory: {memory_needed:.1f}%"
                })
        
        return recommendations
    
    def generate_capacity_report(self, recommendations, current_capacity):
        """生成容量规划报告"""
        report = {
            'report_date': datetime.now().isoformat(),
            'current_capacity': current_capacity,
            'recommendations': recommendations,
            'summary': {
                'total_recommendations': len(recommendations),
                'earliest_recommendation': min([r['date'] for r in recommendations]) if recommendations else None,
                'max_growth_factor': max([r['growth_factor'] for r in recommendations]) if recommendations else 0
            }
        }
        
        return report
    
    def plot_predictions(self, predictions, recommendations):
        """绘制预测图表"""
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))
        
        # CPU预测
        ax1.plot(predictions['days'], predictions['predicted_cpu'], label='Predicted CPU Usage', color='blue')
        ax1.axhline(y=80, color='red', linestyle='--', label='Threshold (80%)')
        ax1.set_title('CPU Usage Prediction')
        ax1.set_xlabel('Days from Now')
        ax1.set_ylabel('CPU Usage (%)')
        ax1.legend()
        ax1.grid(True)
        
        # 内存预测
        ax2.plot(predictions['days'], predictions['predicted_memory'], label='Predicted Memory Usage', color='green')
        ax2.axhline(y=80, color='red', linestyle='--', label='Threshold (80%)')
        ax2.set_title('Memory Usage Prediction')
        ax2.set_xlabel('Days from Now')
        ax2.set_ylabel('Memory Usage (%)')
        ax2.legend()
        ax2.grid(True)
        
        plt.tight_layout()
        plt.savefig('/tmp/capacity_prediction.png')
        plt.show()

if __name__ == "__main__":
    planner = CapacityPlanner()
    
    # 模拟历史数据
    historical_data = [
        {'timestamp': datetime.now() - timedelta(days=i), 'cpu_percent': 60 + i*0.5, 'memory_percent': 55 + i*0.3}
        for i in range(30, 0, -1)
    ]
    
    # 预测未来需求
    predictions = planner.predict_resource_needs(historical_data)
    
    # 生成扩容建议
    current_capacity = {'nodes': 10, 'cpu_cores': 100, 'memory_gb': 400}
    recommendations = planner.calculate_scaling_recommendations(predictions, current_capacity)
    
    # 生成报告
    report = planner.generate_capacity_report(recommendations, current_capacity)
    
    print("容量规划报告:")
    print(f"当前容量: {report['current_capacity']}")
    print(f"建议扩容次数: {report['summary']['total_recommendations']}")
    
    for rec in recommendations[:5]:  # 显示前5个建议
        print(f"  {rec['date'].strftime('%Y-%m-%d')}: 建议扩容 {rec['growth_factor']:.2f}x ({rec['reason']})")
```

## 7. 最佳实践与实施指南

### 7.1 SRE实施原则

```markdown
## ⚙️ SRE实施原则

### 1. 可靠性优先
- 以SLI/SLO为衡量标准
- 建立错误预算管理机制
- 平衡新功能开发与系统稳定性

### 2. 自动化驱动
- 用代码管理运维任务
- 实施无人值守运维
- 建立自愈能力

### 3. 持续改进
- 定期进行事后分析
- 从故障中学习改进
- 持续优化系统架构

### 4. 文化建设
- 建立学习型组织
- 鼓励实验和创新
- 营造无指责文化
```

### 7.2 实施检查清单

```yaml
SRE实施检查清单:
  监控告警:
    ☐ 核心指标监控配置
    ☐ SLI/SLO定义完成
    ☐ 告警规则配置完成
    ☐ 告警通知渠道建立
  
  自动化运维:
    ☐ 基础设施即代码实施
    ☐ 自动部署流水线建立
    ☐ 自愈系统配置完成
    ☐ 备份恢复机制建立
  
  故障响应:
    ☐ 值班制度建立
    ☐ 响应流程文档化
    ☐ 应急预案制定
    ☐ 演练机制建立
  
  变更管理:
    ☐ 变更审批流程建立
    ☐ 发布策略制定
    ☐ 回滚机制配置
    ☐ 测试验证流程建立
  
  容量规划:
    ☐ 资源使用监控建立
    ☐ 容量预测模型建立
    ☐ 扩容策略制定
    ☐ 成本优化机制建立
```

## 8. 未来发展趋势

### 8.1 智能化运维

```yaml
SRE智能化趋势:
  1. AI驱动的异常检测
     - 机器学习异常检测
     - 预测性故障预防
     - 智能容量规划
  
  2. 自主运维系统
     - 无人驾驶运维
     - 自适应系统调优
     - 智能决策支持
  
  3. 混合云SRE
     - 多云统一运维
     - 跨云故障响应
     - 统一监控治理
```

---
*本文档基于企业级SRE实践经验编写，持续更新最新技术和最佳实践。*