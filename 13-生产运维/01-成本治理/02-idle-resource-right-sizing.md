---
title: 空闲资源识别与 Right-Sizing
description: '定义 VPA 推荐应用、闲置 PV/LB 清理、低利用率节点识别及自动化 Right-Sizing Pipeline'
summary: '定义 VPA 推荐应用、闲置 PV/LB 清理、低利用率节点识别及自动化 Right-Sizing Pipeline'
category: production-operations
tags:
- production
- operations
- finops
- right-sizing
- vpa
- idle-resources
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Right-Sizing 是什么
- 如何 识别空闲资源
- 如何 配置 VPA
trigger_keywords:
- right-sizing
- vpa
- idle
- resource
- optimization
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 空闲资源识别与 Right-Sizing

## 1. 空闲资源概述

### 1.1 空闲资源类型

```
Kubernetes 空闲资源分类:

1. 计算资源闲置
   - Pod Request 远大于实际使用
   - 节点利用率低
   - 未调度的 Reserved 资源

2. 存储资源闲置
   - 未挂载的 PV
   - 使用率极低的 PV
   - 废弃的 PVC

3. 网络资源闲置
   - 未使用的 LoadBalancer Service
   - 未关联的 Elastic IP
   - 空闲的 NAT Gateway

4. 集群级闲置
   - 空节点（无 Pod 调度）
   - 低利用率节点组
   - 过大的节点规格
```

### 1.2 闲置资源成本影响

```
典型闲置资源浪费比例:

计算闲置: 30-50%（Request vs 实际使用）
存储闲置: 20-40%（分配 vs 实际使用）
网络闲置: 10-20%（未使用的 LB）

年化成本影响（¥1000万/年集群）:
  计算闲置: ¥300-500万
  存储闲置: ¥60-120万
  网络闲置: ¥20-40万

总计: ¥380-660万/年 可优化空间
```

## 2. VPA（Vertical Pod Autoscaler）

### 2.1 VPA 工作原理

```
VPA 架构:

┌─────────────────────────────────────────────┐
│                VPA Recommender               │
│  - 采集历史资源使用数据                       │
│  - 计算最优 Request/Limit                    │
│  - 生成推荐值                                │
└────────────────────┬────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│                VPA Updater                   │
│  - 检测 Pod 是否偏离推荐值                   │
│  - 驱逐需要更新的 Pod                        │
│  - (仅在 Auto 模式下)                        │
└────────────────────┬────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│                VPA Admission Controller      │
│  - 拦截 Pod 创建请求                         │
│  - 注入推荐的 Request/Limit                  │
│  - (仅在 Auto/Recreate 模式下)               │
└─────────────────────────────────────────────┘
```

### 2.2 VPA 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 VPA
git clone https://github.com/kubernetes/autoscaler.git
cd autoscaler/vertical-pod-autoscaler
./hack/vpa-up.sh

# 验证安装
kubectl get pods -n kube-system | grep vpa
```
### 2.3 VPA 配置模式

```yaml
# VPA Recommendation Only 模式（推荐初始使用）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Off"  # Off = 仅推荐，不自动应用
  resourcePolicy:
    containerPolicies:
      - containerName: "*"
        minAllowed:
          cpu: "100m"
          memory: "128Mi"
        maxAllowed:
          cpu: "8"
          memory: "16Gi"
        controlledResources: ["cpu", "memory"]
```

```yaml
# VPA Auto 模式（生产环境谨慎使用）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Auto"  # 自动应用推荐值
    minReplicas: 2       # 确保至少 2 副本（避免全部重启）
  resourcePolicy:
    containerPolicies:
      - containerName: "*"
        minAllowed:
          cpu: "100m"
          memory: "128Mi"
        maxAllowed:
          cpu: "8"
          memory: "16Gi"
        controlledResources: ["cpu", "memory"]
```

### 2.4 VPA 推荐值查询

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VPA 推荐值
kubectl describe vpa my-app-vpa -n production

# 输出示例:
# Status:
#   Recommendation:
#     Container Recommendations:
#       Container Name: my-app
#       Lower Bound:     # 最低推荐值
#         Cpu: 200m
#         Memory: 256Mi
#       Target:          # 目标推荐值
#         Cpu: 500m
#         Memory: 512Mi
#       Upper Bound:     # 最高推荐值
#         Cpu: 1
#         Memory: 1Gi
#       Uncapped Target: # 无限制时的推荐值
#         Cpu: 2
#         Memory: 2Gi
```
### 2.5 VPA 推荐值批量导出

```python
# vpa_recommendations_export.py
import subprocess
import json
import csv

def export_vpa_recommendations():
    """导出所有 VPA 推荐值"""
    result = subprocess.run(
        ["kubectl", "get", "vpa", "--all-namespaces", "-o", "json"],
        capture_output=True, text=True
    )
    vpas = json.loads(result.stdout)

    recommendations = []
    for vpa in vpas["items"]:
        ns = vpa["metadata"]["namespace"]
        name = vpa["metadata"]["name"]
        target_ref = vpa["spec"]["targetRef"]

        if "recommendation" in vpa.get("status", {}):
            for container in vpa["status"]["recommendation"]["containerRecommendations"]:
                recommendations.append({
                    "namespace": ns,
                    "vpa_name": name,
                    "target_kind": target_ref["kind"],
                    "target_name": target_ref["name"],
                    "container": container["containerName"],
                    "cpu_target": container["target"]["cpu"],
                    "memory_target": container["target"]["memory"],
                    "cpu_lower": container["lowerBound"]["cpu"],
                    "memory_lower": container["lowerBound"]["memory"],
                    "cpu_upper": container["upperBound"]["cpu"],
                    "memory_upper": container["upperBound"]["memory"],
                })

    # 写入 CSV
    with open("vpa_recommendations.csv", "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=recommendations[0].keys())
        writer.writeheader()
        writer.writerows(recommendations)

    return recommendations
```

## 3. 闲置 PV/LB 清理

### 3.1 未挂载 PV 识别

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找未挂载的 PV
kubectl get pv -o json | jq -r '
  .items[] |
  select(.status.phase == "Available" or .spec.claimRef == null) |
  {
    name: .metadata.name,
    capacity: .spec.capacity.storage,
    storageClass: .spec.storageClassName,
    reclaimPolicy: .spec.persistentVolumeReclaimPolicy,
    age: .metadata.creationTimestamp
  }
'

# 查找长期未使用的 PV（> 30 天）
kubectl get pv -o json | jq -r '
  .items[] |
  select(.status.phase == "Available") |
  select((now - (.metadata.creationTimestamp | fromdateiso8601)) > 2592000) |
  {
    name: .metadata.name,
    capacity: .spec.capacity.storage,
    age_days: ((now - (.metadata.creationTimestamp | fromdateiso8601)) / 86400 | floor)
  }
'
```
### 3.2 PVC 使用率监控

```yaml
# Prometheus Rule: PVC 使用率低告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pvc-usage-alerts
spec:
  groups:
    - name: pvc-usage
      rules:
        - alert: PVCLowUsage
          expr: |
            (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) < 0.1
            and kubelet_volume_stats_capacity_bytes > 0
          for: 30d
          labels:
            severity: info
            team: platform
          annotations:
            summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率低于 10%"
            description: "Namespace {{ $labels.namespace }} 的 PVC 已连续 30 天使用率低于 10%，建议清理"
```

### 3.3 未使用 LoadBalancer 识别

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找未使用的 LoadBalancer Service
kubectl get svc --all-namespaces -o json | jq -r '
  .items[] |
  select(.spec.type == "LoadBalancer") |
  select(.status.loadBalancer.ingress == null or .status.loadBalancer.ingress == []) |
  {
    namespace: .metadata.namespace,
    name: .metadata.name,
    age: .metadata.creationTimestamp
  }
'

# 查找没有 Endpoints 的 LoadBalancer Service
kubectl get svc --all-namespaces -o json | jq -r '
  .items[] |
  select(.spec.type == "LoadBalancer") |
  {
    namespace: .metadata.namespace,
    name: .metadata.name,
    selector: .spec.selector
  }
' | while read -r line; do
  ns=$(echo "$line" | jq -r '.namespace')
  name=$(echo "$line" | jq -r '.name')
  selector=$(echo "$line" | jq -r '.selector | to_entries | map("\(.key)=\(.value)") | join(",")')
  endpoints=$(kubectl get endpoints "$name" -n "$ns" -o json 2>/dev/null | jq '.subsets | length')
  if [ "$endpoints" = "0" ] || [ "$endpoints" = "null" ]; then
    echo "UNUSED: $ns/$name"
  fi
done
```
### 3.4 自动清理脚本

```yaml
# CronJob: 每周清理闲置资源报告
apiVersion: batch/v1
kind: CronJob
metadata:
  name: idle-resource-report
  namespace: platform
spec:
  schedule: "0 9 * * 1"  # 每周一 9:00
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: resource-reader
          containers:
            - name: reporter
              image: python:3.11-slim
              command:
                - /bin/bash
                - -c
                - |
                  pip install kubernetes
                  python /scripts/idle_resource_report.py
              volumeMounts:
                - name: scripts
                  mountPath: /scripts
          volumes:
            - name: scripts
              configMap:
                name: idle-resource-scripts
          restartPolicy: OnFailure
```

## 4. 低利用率节点识别

### 4.1 节点利用率查询

```promql
# 节点 CPU 利用率（过去 7 天平均）
1 - avg by (instance) (
  rate(node_cpu_seconds_total{mode="idle"}[7d])
)

# 节点内存利用率
1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)

# 低利用率节点（CPU < 30% 且 Memory < 40%）
(
  1 - avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[7d]))
) < 0.3
and
(
  1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)
) < 0.4
```

### 4.2 节点效率评分

```python
# node_efficiency_score.py
def calculate_node_efficiency(node_metrics):
    """计算节点效率评分（0-100）"""
    cpu_util = node_metrics["cpu_utilization"]
    memory_util = node_metrics["memory_utilization"]
    pod_count = node_metrics["pod_count"]
    allocatable_cpu = node_metrics["allocatable_cpu"]
    allocatable_memory = node_metrics["allocatable_memory"]

    # CPU 效率（权重 40%）
    cpu_score = min(cpu_util / 0.7, 1.0) * 40

    # Memory 效率（权重 40%）
    memory_score = min(memory_util / 0.8, 1.0) * 40

    # Pod 密度（权重 20%）
    # 假设每核心理想 Pod 数为 10
    ideal_pods_per_core = 10
    pod_density = pod_count / allocatable_cpu
    pod_score = min(pod_density / ideal_pods_per_core, 1.0) * 20

    total_score = cpu_score + memory_score + pod_score

    return {
        "node": node_metrics["name"],
        "score": round(total_score, 1),
        "cpu_score": round(cpu_score, 1),
        "memory_score": round(memory_score, 1),
        "pod_score": round(pod_score, 1),
        "recommendation": get_recommendation(total_score)
    }

def get_recommendation(score):
    if score < 30:
        return "考虑下线或缩容节点组"
    elif score < 50:
        return "考虑迁移到更小规格节点"
    elif score < 70:
        return "效率一般，有优化空间"
    else:
        return "效率良好"
```

### 4.3 节点组 Right-Sizing

```yaml
# Cluster Autoscaler 配置优化
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
        - name: cluster-autoscaler
          image: k8s.gcr.io/autoscaling/cluster-autoscaler:v1.28.0
          command:
            - ./cluster-autoscaler
            - --v=4
            - --cloud-provider=aws
            - --skip-nodes-with-local-storage=false
            - --expander=least-waste  # 选择浪费最少的节点组
            - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/prod-cluster
            - --scale-down-utilization-threshold=0.4  # 利用率 < 40% 考虑缩容
            - --scale-down-unneeded-time=30m  # 30 分钟不需要则缩容
            - --scale-down-delay-after-add=10m
```

## 5. 自动化 Right-Sizing Pipeline

### 5.1 Pipeline 架构

```
Right-Sizing Pipeline:

┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  数据采集    │───▶│  分析引擎    │───▶│  推荐生成    │
│ Prometheus  │    │  计算推荐值  │    │  报告 + PR  │
└─────────────┘    └─────────────┘    └─────────────┘
                                            │
                                            ▼
                                    ┌─────────────┐
                                    │  审批流程    │
                                    │  人工确认    │
                                    └─────────────┘
                                            │
                                            ▼
                                    ┌─────────────┐
                                    │  自动应用    │
                                    │  GitOps 同步 │
                                    └─────────────┘
```

### 5.2 推荐值计算算法

```python
# right_sizing_calculator.py
import numpy as np
from prometheus_api_client import PrometheusConnect

class RightSizingCalculator:
    def __init__(self, prometheus_url):
        self.prom = PrometheusConnect(url=prometheus_url)

    def calculate_recommendation(self, namespace, deployment, container,
                                  window_days=14, percentile=95):
        """计算 Right-Sizing 推荐值"""
        # 查询历史 CPU 使用
        cpu_query = f'''
            quantile_over_time({percentile/100},
                rate(container_cpu_usage_seconds_total{
                    namespace="{namespace}",
                    pod=~"{deployment}-.*",
                    container="{container}"
                }[5m])
            [{window_days}d:]
        )
        '''
        cpu_usage = self.prom.custom_query(cpu_query)

        # 查询历史 Memory 使用
        memory_query = f'''
            quantile_over_time({percentile/100},
                container_memory_working_set_bytes{
                    namespace="{namespace}",
                    pod=~"{deployment}-.*",
                    container="{container}"
                }
            [{window_days}d:]
        )
        '''
        memory_usage = self.prom.custom_query(memory_query)

        # 计算推荐值（加上 20% buffer）
        cpu_recommendation = float(cpu_usage[0]["value"][1]) * 1.2
        memory_recommendation = float(memory_usage[0]["value"][1]) * 1.2

        # 转换为 K8s 资源格式
        return {
            "cpu_request": self._format_cpu(cpu_recommendation),
            "memory_request": self._format_memory(memory_recommendation),
            "cpu_limit": self._format_cpu(cpu_recommendation * 2),
            "memory_limit": self._format_memory(memory_recommendation * 1.5),
        }

    def _format_cpu(self, cores):
        if cores < 1:
            return f"{int(cores * 1000)}m"
        return f"{cores:.1f}"

    def _format_memory(self, bytes_val):
        gib = bytes_val / (1024**3)
        if gib < 1:
            mib = bytes_val / (1024**2)
            return f"{int(mib)}Mi"
        return f"{gib:.1f}Gi"
```

### 5.3 GitOps 集成

```python
# right_sizing_pr_generator.py
import yaml
import subprocess

def generate_right_sizing_pr(namespace, deployment, container, recommendations):
    """生成 Right-Sizing PR"""
    # 读取当前部署配置
    result = subprocess.run(
        ["kubectl", "get", "deployment", deployment, "-n", namespace, "-o", "yaml"],
        capture_output=True, text=True
    )
    current_config = yaml.safe_load(result.stdout)

    # 更新资源配额
    for container_spec in current_config["spec"]["template"]["spec"]["containers"]:
        if container_spec["name"] == container:
            container_spec["resources"] = {
                "requests": {
                    "cpu": recommendations["cpu_request"],
                    "memory": recommendations["memory_request"]
                },
                "limits": {
                    "cpu": recommendations["cpu_limit"],
                    "memory": recommendations["memory_limit"]
                }
            }

    # 写入文件
    output_path = f"right-sizing/{namespace}/{deployment}.yaml"
    with open(output_path, "w") as f:
        yaml.dump(current_config, f, default_flow_style=False)

    # 创建 PR
    subprocess.run(["git", "add", output_path])
    subprocess.run([
        "git", "commit", "-m",
        f"right-sizing: {namespace}/{deployment}/{container}\n"
        f"CPU: {recommendations['cpu_request']} (was: {get_current_value('cpu')})\n"
        f"Memory: {recommendations['memory_request']} (was: {get_current_value('memory')})"
    ])

    return output_path
```

## 6. Right-Sizing 推广策略

### 6.1 分阶段推广

```
Right-Sizing 推广计划:

Phase 1: 试点（第 1-2 周）
  - 选择 3-5 个无状态服务
  - 使用 VPA Recommendation Only 模式
  - 验证推荐值准确性

Phase 2: 扩大（第 3-4 周）
  - 扩展到所有无状态服务
  - 开始自动应用（低风险服务）
  - 收集反馈

Phase 3: 全面推广（第 5-8 周）
  - 覆盖所有服务（含有状态）
  - 集成到 CI/CD Pipeline
  - 建立持续优化机制
```

### 6.2 风险控制

```
Right-Sizing 风险控制:

安全措施:
  1. 设置最小资源下限
  2. 保留 20% buffer
  3. 分批应用（先 1 个 Pod，观察后全量）
  4. 保留回滚配置

排除列表:
  - 核心关键服务（手动评估）
  - 已知有突发流量的服务
  - 最近 7 天有过 OOM 的服务

监控指标:
  - 应用延迟（P99）
  - 错误率
  - OOM 事件
  - CPU Throttling
```

---

*本文档定义空闲资源识别和 Right-Sizing 的完整方案。平台团队应定期执行资源审计，推动各团队优化资源配置。*


<!-- risk-assessed -->
