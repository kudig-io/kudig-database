---
title: KubeElastic (entities)
description: '## 概述'
summary: 'KubeElastic 是一个 Kubernetes 原生的弹性伸缩和资源优化平台，专注于基于实时负载和成本的智能资源调整。它结合机器学习预测算法，自动调整 Pod 资源配额（VPA）和副本数（HPA），同时优化集群节点利用率，帮助用户在保证性能 SLO 的前提下降低云成本。'
category: entities
tags:
- k8s
- cncf
- observability
- kubeelasti
- prometheus
- grafana
- hpa
- vpa
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeElastic 是什么
- 如何 KubeElastic
trigger_keywords:
- KubeElastic
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeElastic

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

KubeElasti 是一个 CNCF 沙箱项目，旨在为 Kubernetes 提供弹性存储卷管理能力。它通过动态调整 PV 大小和 IOPS 限制，根据工作负载实际需求自动伸缩存储资源。KubeElasti 解决了 K8s 存储资源过度分配的问题——许多 PV 在创建时分配了大量空间但实际使用率很低，KubeElasti 可以根据监控指标自动调整存储分配，降低存储成本。

## Key Features（核心能力）

- **动态卷扩缩**：根据使用率自动扩展或回收 PV 空间
- **IOPS 调整**：动态调整云存储卷的 IOPS 和吞吐限制
- **基于指标的伸缩**：通过 Prometheus 指标触发存储伸缩
- **多 CSI 支持**：兼容支持 Volume Expansion 的 CSI 驱动
- **安全策略**：定义最小/最大卷大小限制防止异常伸缩
- **通知机制**：伸缩事件通知到 Slack/PagerDuty

## 架构与工作原理

KubeElasti 由 Controller 和 Monitor 组成。Controller 监听 ElasticVolume CRD，管理卷伸缩的生命周期。Monitor 定期从 Prometheus 查询 PV 使用率指标，当使用率超过/低于阈值时触发伸缩决策。Controller 通过 K8s Volume Expansion API（editting PVC spec.resources.requests.storage）和 CSI 驱动接口执行实际的卷大小调整。

## K8s 集成

KubeElasti 通过自定义 CRD 与 K8s 集成。ElasticVolume CRD 定义目标 PVC、伸缩策略（阈值、最小/最大大小、步长）。Controller 监听这些 CRD 和 Prometheus 指标，通过修改 PVC 的 resources.requests.storage 字段触发 CSI Volume Expansion。仅支持 allowVolumeExpansion: true 的 StorageClass。

## 生产用例

- **存储成本优化**：自动回收未使用的 PV 空间
- **数据库存储管理**：根据数据库增长自动扩展存储
- **日志存储管理**：根据日志量自动调整日志卷大小
- **开发环境**：为开发环境自动分配和回收存储

## 安装与配置

```bash
# 🟢 安装 KubeElasti
kubectl apply -f https://github.com/kubeelasti/kubeelasti/releases/latest/download/kubeelasti.yaml

# 🟢 验证安装
kubectl get pods -n kubeelasti-system
kubectl get crd | grep kubeelasti

# 🟢 查看 Controller 状态
kubectl get pods -n kubeelasti-system -l app=kubeelasti-controller

# 🟢 查看 ElasticVolume 资源
kubectl get elasticvolume -A
```

### ElasticVolume CRD 示例

```yaml
apiVersion: kubeelasti.io/v1alpha1
kind: ElasticVolume
metadata:
  name: postgres-data-elastic
  namespace: database
spec:
  # 目标 PVC
  pvcName: data-postgresql-0
  # 伸缩策略
  scalingPolicy:
    # 扩展阈值: 使用率超过 80% 时扩展
    scaleUpThreshold: 80
    # 收缩阈值: 使用率低于 30% 时收缩
    scaleDownThreshold: 30
    # 每次扩展步长
    scaleUpStep: 20Gi
    # 每次收缩步长
    scaleDownStep: 10Gi
    # 最小大小
    minSize: 50Gi
    # 最大大小
    maxSize: 500Gi
    # 冷却时间 (避免频繁伸缩)
    cooldownPeriod: 1h
  # 监控指标源
  metricsSource:
    type: prometheus
    url: http://prometheus.monitoring.svc:9090
    query: |
      kubelet_volume_stats_used_bytes{persistentvolumeclaim="data-postgresql-0"} 
      / kubelet_volume_stats_capacity_bytes{persistentvolumeclaim="data-postgresql-0"} * 100
  # 通知配置
  notifications:
    slack:
      webhookUrl: https://hooks.slack.com/services/xxx
      channel: "#storage-alerts"
```

### StorageClass 要求

```yaml
# StorageClass 必须支持 Volume Expansion
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: elastic-storage
provisioner: ebs.csi.aws.com
allowVolumeExpansion: true  # 必须为 true
parameters:
  type: gp3
  encrypted: "true"
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 ElasticVolume 状态
kubectl get elasticvolume -A
kubectl describe elasticvolume postgres-data-elastic -n database

# 🟢 查看 Controller 日志
kubectl logs -n kubeelasti-system -l app=kubeelasti-controller --tail=50

# 🟢 查看 PVC 当前大小
kubectl get pvc data-postgresql-0 -n database

# 🟢 查看伸缩事件
kubectl get events -n database --field-selector reason=VolumeResized

# 🟡 手动触发扩展
kubectl patch pvc data-postgresql-0 -n database \
  -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 🟡 暂停自动伸缩
kubectl annotate elasticvolume postgres-data-elastic \
  kubeelasti.io/paused=true -n database

# 🟡 恢复自动伸缩
kubectl annotate elasticvolume postgres-data-elastic \
  kubeelasti.io/paused- -n database
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 未触发扩展 | 阈值未达到/指标查询失败 | `kubectl describe elasticvolume` | 检查 Prometheus 查询和阈值 |
| 扩展失败 | CSI 不支持/达到上限 | `kubectl get events` | 检查 StorageClass 和 maxSize |
| 收缩失败 | 文件系统不支持收缩 | 查看 Controller 日志 | 大多数 CSI 不支持收缩，仅扩展 |
| 指标不可用 | Prometheus 不可达 | `curl prometheus:9090/-/healthy` | 检查 Prometheus 连接 |
| 频繁伸缩 | 冷却时间太短 | 查看伸缩历史 | 增加 cooldownPeriod |

### 排查流程

```
1. kubectl get elasticvolume → 确认资源状态
2. kubectl describe elasticvolume → 查看伸缩策略和状态
3. kubectl logs -l app=kubeelasti-controller → 查看决策日志
4. 验证 Prometheus 指标查询正常
5. 检查 StorageClass 是否支持 Volume Expansion
```

## 生产案例

### 案例1: 数据库存储自动扩展
- **场景**: PostgreSQL 数据库存储增长不可预测，手动扩展不及时导致磁盘满
- **方案**: KubeElasti 监控 PVC 使用率，超过 80% 自动扩展 20Gi
- **效果**: 消除磁盘满故障，存储管理完全自动化

### 案例2: 存储成本优化
- **场景**: 开发环境 PVC 过度分配，实际使用率仅 20%
- **方案**: KubeElasti 根据实际使用调整存储分配
- **效果**: 存储成本降低 40%，资源利用率提升

## 对比替代方案

| 维度 | KubeElasti | 手动扩展 | K8s VPA | 云厂商方案 |
|------|-----------|---------|---------|----------|
| 自动化 | 全自动 | 手动 | 仅 CPU/内存 | 部分自动 |
| 存储伸缩 | 支持 | 支持 | 不支持 | 有限 |
| 基于指标 | 支持 | 无 | 支持 | 有限 |
| 多 CSI | 支持 | N/A | N/A | 单一 |
| 成本 | 免费 | 免费 | 免费 | 付费 |

## 检查清单

- [ ] StorageClass 设置 allowVolumeExpansion: true
- [ ] ElasticVolume 配置了合理的 min/max 限制
- [ ] Prometheus 指标查询已验证
- [ ] 冷却时间合理 (避免频繁伸缩)
- [ ] 通知机制已配置 (Slack/PagerDuty)
- [ ] 监控伸缩事件和存储使用率
- [ ] 定期审查伸缩策略有效性

## Related

- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeelasti
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
