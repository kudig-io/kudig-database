---
title: Terway 运维手册
description: '# Terway 运维手册'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- prometheus
- grafana
- cilium
- networkpolicy
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 运维手册 是什么
- 如何 Terway 运维手册
trigger_keywords:
- Terway
- 运维手册
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 运维手册

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 04 - Terway 运维手册 (Operations Manual)

## 技术细节

### 监控与指标

#### Terway 核心指标

| 指标名称 | 类型 | 说明 |
|---------|------|------|
| `terway_eni_count` | Gauge | 节点 ENI 数量 |
| `terway_eni_ip_count` | Gauge | ENI 辅助 IP 数量 |
| `terway_ip_pool_size` | Gauge | IP 池大小 |
| `terway_ip_allocated` | Gauge | 已分配 IP 数 |
| `terway_api_latency` | Histogram | 阿里云 API 延迟 |
| `terway_api_errors_total` | Counter | API 错误总数 |
| `terway_cni_add_latency` | Histogram | CNI ADD 延迟 |
| `terway_cni_del_latency` | Histogram | CNI DEL 延迟 |

#### 配置 ServiceMonitor

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: terway-monitor
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: terway-eniip
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics
```

#### PrometheusRule 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-alerts
  namespace: kube-system
spec:
  groups:
    - name: terway.rules
      rules:
        # IP 池即将耗尽
        - alert: TerwayIPPoolLow
          expr: |
            terway_ip_allocated / terway_ip_pool_size > 0.8
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} IP 池使用率超过 80%"

        # ENI 配额即将用尽
        - alert: TerwayENIQuotaLow
          expr: |
            terway_eni_count / terway_eni_quota > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} ENI 配额使用率超过 90%"

        # API 错误率过高
        - alert: TerwayAPIErrorRateHigh
          expr: |
            rate(terway_api_errors_total[5m]) > 0.1
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Terway API 错误率过高"

        # CNI 延迟过高
        - alert: TerwayCNILatencyHigh
          expr: |
            histogram_quantile(0.99, rate(terway_cni_add_latency_bucket[5m])) > 5
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "CNI ADD P99 延迟超过 5 秒"
```

### 日志管理

#### 日志位置

```bash
# Terway DaemonSet 日志
kubectl logs -n kube-system -l app=terway-eniip -f

# 节点上的 Terway 日志
journalctl -u terway -f

# CNI 插件日志
cat /var/log/terway/cni.log

# Terway Controller 日志
kubectl logs -n kube-system -l app=terway-controlplane -f
```

#### 日志级别调整

```bash
# 🟡 中风险：临时调整日志级别
kubectl set env ds/terway-eniip -n kube-system LOG_LEVEL=debug

# 恢复默认级别
kubectl set env ds/terway-eniip -n kube-system LOG_LEVEL=info
```

### 升级与回滚

#### 升级 Terway

```bash
# 🟢 低风险：检查当前版本
kubectl get ds -n kube-system terway-eniip -o jsonpath='{.spec.template.spec.containers[0].image}'

# 🟡 中风险：升级 Terway (通过 ACK 控制台或 kubectl)
# 方式 1: ACK 控制台 → 组件管理 → Terway → 升级
# 方式 2: kubectl
kubectl set image ds/terway-eniip -n kube-system terway=registry.cn-hangzhou.aliyuncs.com/acs/terway:v1.8.0

# 🟢 低风险：监控升级进度
kubectl rollout status ds/terway-eniip -n kube-system
```

#### 回滚 Terway

```bash
# 🔴 高风险：回滚 Terway
# 1. 查看历史版本
kubectl rollout history ds/terway-eniip -n kube-system

# 2. 回滚到上一版本
kubectl rollout undo ds/terway-eniip -n kube-system

# 3. 监控回滚进度
kubectl rollout status ds/terway-eniip -n kube-system
```

### 日常运维操作

#### 检查 Terway 健康状态

```bash
#!/bin/bash
# 🟢 低风险：Terway 健康检查脚本
set -euo pipefail

echo "=== Terway 健康检查 $(date) ==="

# 1. 检查 Terway Pod 状态
echo "[1] Terway Pod 状态:"
kubectl get pods -n kube-system -l app=terway-eniip -o wide

# 2. 检查 Terway Controller 状态
echo "[2] Terway Controller 状态:"
kubectl get pods -n kube-system -l app=terway-controlplane

# 3. 检查 ENI 使用情况
echo "[3] ENI 使用情况:"
kubectl get podeni -A --no-headers | wc -l
echo "  总 PodENI 数量"

# 4. 检查 Failed 状态
echo "[4] 异常状态:"
kubectl get podeni -A --field-selector status.phase=Failed 2>/dev/null || echo "  无异常"

# 5. 检查 Terway 指标
echo "[5] Terway 指标:"
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway-eniip -o name | head -1) -- \
  curl -s http://localhost:19090/metrics | grep -E "^terway_" | head -10

echo "=== 检查完成 ==="
```

#### 清理孤儿资源

```bash
#!/bin/bash
# 🟡 中风险：清理孤儿 PodENI 资源
set -euo pipefail

echo "=== 清理孤儿 PodENI ==="

# 查找孤儿 PodENI
kubectl get podeni -A -o json | jq -r '.items[] | select(.status.phase=="Bound") | "\(.metadata.namespace)/\(.metadata.name)/\(.spec.podName)"' | while IFS='/' read ns name pod; do
  if ! kubectl get pod $pod -n $ns &>/dev/null; then
    echo "发现孤儿: $ns/$name (Pod: $pod)"
    # kubectl delete podeni $name -n $ns  # 取消注释以删除
  fi
done

echo "=== 清理完成 ==="
```

### 备份与恢复

#### 备份 Terway 配置

```bash
# 🟢 低风险：备份 Terway 配置
kubectl get cm -n kube-system eni-config -o yaml > terway-config-backup-$(date +%Y%m%d).yaml
kubectl get ds -n kube-system terway-eniip -o yaml > terway-ds-backup-$(date +%Y%m%d).yaml
kubectl get deploy -n kube-system terway-controlplane -o yaml > terway-controller-backup-$(date +%Y%m%d).yaml
```

#### 恢复 Terway 配置

```bash
# 🔴 高风险：恢复 Terway 配置
# 1. 应用备份配置
kubectl apply -f terway-config-backup-20260711.yaml

# 2. 重启 Terway
kubectl rollout restart ds/terway-eniip -n kube-system
kubectl rollout restart deploy/terway-controlplane -n kube-system

# 3. 验证
kubectl get pods -n kube-system -l app=terway-eniip
```

### 故障应急

#### Terway 完全重启

```bash
# 🔴 高风险：Terway 完全重启（可能导致短暂网络中断）
# 1. 备份配置
kubectl get cm -n kube-system eni-config -o yaml > /tmp/eni-config-backup.yaml

# 2. 删除所有 Terway Pod
kubectl delete pods -n kube-system -l app=terway-eniip
kubectl delete pods -n kube-system -l app=terway-controlplane

# 3. 等待重建
kubectl wait --for=condition=Ready pod -n kube-system -l app=terway-eniip --timeout=300s

# 4. 验证
kubectl get pods -n kube-system -l app=terway-eniip
```

#### 单节点网络修复

```bash
# 🟡 中风险：单节点 Terway 修复
NODE_NAME=<node-name>

# 1. 驱逐节点上的 Pod
kubectl drain $NODE_NAME --ignore-daemonsets --delete-emptydir-data

# 2. 重启节点上的 Terway
kubectl delete pod -n kube-system -l app=terway-eniip --field-selector spec.nodeName=$NODE_NAME

# 3. 等待 Terway 就绪
kubectl wait --for=condition=Ready pod -n kube-system -l app=terway-eniip --field-selector spec.nodeName=$NODE_NAME --timeout=120s

# 4. 恢复调度
kubectl uncordon $NODE_NAME
```

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]

## Related

- [[antrea]] — Antrea
- [[40-terway-product-overview]] — Terway 产品概览
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 44-terway-operations-manual

<!-- risk-assessed -->
