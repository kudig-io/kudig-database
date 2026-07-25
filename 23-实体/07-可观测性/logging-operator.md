---
title: Logging Operator [entities]
description: '## 概述'
summary: 'Logging Operator 是一个 Kubernetes Operator，用于自动化部署和配置 Kubernetes 集群的日志收集管道。它基于 Fluentd 和 Fluent Bit 构建，通过 CRD 声明式地管理日志的收集、过滤、转换和路由，支持将日志发送到 Elasticsearch、Loki、S3、Kafka 等多种后端。'
category: entities
tags:
- k8s
- cncf
- observability
- logging-operator
- prometheus
- grafana
- kafka
- elasticsearch
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Logging Operator 是什么
- 如何 Logging Operator
trigger_keywords:
- Logging
- Operator
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Logging Operator

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Logging Operator 是由 Banzaicloud（现 Cisco）开发的 Kubernetes 日志收集管道 Operator，2021 年加入 CNCF Sandbox。它基于 Fluentd 和 Fluent Bit 构建，通过 CRD 声明式地管理日志的收集、过滤、转换和路由，支持将日志发送到 Elasticsearch、Loki、S3、Kafka、CloudWatch 等多种后端。Logging Operator 将复杂的日志管道配置转化为 Kubernetes 原生的声明式 API。

## 核心特性

- **双层架构**: Fluent Bit（轻量采集器） + Fluentd（处理路由）
- **CRD 声明式管理**: Logging、Flow、Output、ClusterFlow、ClusterOutput CRD
- **多输出后端**: Elasticsearch、Loki、S3、Kafka、CloudWatch、Datadog 等
- **日志过滤与转换**: 支持正则匹配、JSON 解析、字段重命名、记录修改
- **多租户隔离**: 命名空间级别 Flow 隔离和全局 ClusterFlow
- **缓冲保护**: PVC 持久化缓冲区防止输出不可用时数据丢失

## 架构

Logging Operator 采用双层架构。第一层 Fluent Bit 以 DaemonSet 运行在每个节点上，从容器日志文件（/var/log/containers）采集日志，发送到 Fluentd。第二层 Fluentd 以 StatefulSet 运行，负责日志处理（过滤、解析、转换）和路由（发送到多种输出）。Operator 监听 Logging、Flow、Output CRD，将用户配置翻译为 Fluent Bit 和 Fluentd 的配置文件。Flow CRD 定义命名空间级日志处理管道，Output CRD 定义日志输出目标。

## Kubernetes 集成

Logging Operator 通过 CRD 声明式管理日志管道。`logging` CRD 定义全局配置（Fluent Bit/Fluentd 部署参数）。`flow` CRD 为每个命名空间定义日志过滤和路由规则。`output` CRD 定义后端连接配置。Operator 自动管理 Fluent Bit DaemonSet 和 Fluentd StatefulSet 的生命周期。通过 Kubernetes RBAC 控制不同命名空间的日志管道配置权限。

## 生产使用场景

1. **统一日志收集**: 为集群中所有应用提供统一的日志采集和处理管道
2. **多租户日志**: 不同命名空间的日志发送到不同的 Elasticsearch 索引
3. **热温冷分层**: 热数据发往 Elasticsearch/Loki，冷数据归档到 S3
4. **Kafka 管道**: 将日志发送到 Kafka，由下游消费者异步处理

## 安装与配置

```bash
# Helm 安装 Logging Operator
helm repo add banzaicloud-stable https://kubernetes-charts.banzaicloud.com
helm install logging-operator banzaicloud-stable/logging-operator \
  -n logging --create-namespace \
  --set monitoring.serviceMonitor.enabled=true

# 等待 Operator 就绪
kubectl wait --for=condition=available deployment/logging-operator -n logging --timeout=120s
```

```yaml
# Logging CRD（全局配置）
apiVersion: logging.banzaicloud.io/v1beta1
kind: Logging
metadata:
  name: cluster-logging
spec:
  fluentd:
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: "2"
        memory: 2Gi
    bufferStorageVolume:
      pvc:
        spec:
          accessModes: [ReadWriteOnce]
          resources:
            requests:
              storage: 20Gi
  fluentbit:
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
  controlNamespace: logging
---
# Flow CRD（命名空间级日志管道）
apiVersion: logging.banzaicloud.io/v1beta1
kind: Flow
metadata:
  name: app-logs
  namespace: production
spec:
  match:
  - select:
      labels:
        app: payment-service
  filters:
  - parser:
      key_name: log
      parse:
        type: json
  - record_modifier:
      records:
      - cluster: prod-cluster
      - environment: production
  localOutputRefs:
  - loki-output
  - s3-archive
---
# Output CRD（Loki 输出）
apiVersion: logging.banzaicloud.io/v1beta1
kind: Output
metadata:
  name: loki-output
  namespace: production
spec:
  loki:
    url: http://loki.logging.svc:3100
    labels:
      app: "{{.kubernetes.labels.app}}"
      namespace: "{{.kubernetes.namespace_name}}"
    buffer:
      type: file
      path: /buffers/loki
      flush_interval: 10s
      retry_max_interval: 300s
---
# ClusterOutput（S3 归档）
apiVersion: logging.banzaicloud.io/v1beta1
kind: ClusterOutput
metadata:
  name: s3-archive
spec:
  s3:
    s3_bucket: company-logs-archive
    s3_region: us-east-1
    path: "logs/%Y/%m/%d/"
    buffer:
      timekey: 1h
      timekey_wait: 10m
```

## 运维操作

```bash
# 🟢 低风险：查看日志管道状态
kubectl get logging -A
kubectl get flows -A
kubectl get outputs -A
kubectl get clusterflows -A

# 🟢 低风险：查看 Fluentd/Fluent Bit 状态
kubectl get pods -n logging -l app.kubernetes.io/name=fluentd
kubectl get pods -n logging -l app.kubernetes.io/name=fluent-bit
kubectl logs -l app.kubernetes.io/name=fluentd -n logging --tail=50

# 🟡 中风险：更新 Flow 配置
kubectl apply -f updated-flow.yaml

# 🟡 中风险：重启 Fluentd（重新加载配置）
kubectl rollout restart statefulset/fluentd -n logging

# 🔴 高风险：删除 Logging（停止所有日志收集）
kubectl delete logging cluster-logging
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 日志未收集 | Flow match 不匹配 | `kubectl describe flow <name> -n <ns>` | 检查 labels 选择器 |
| Fluentd OOMKilled | 缓冲区过大/内存不足 | `kubectl describe pod -l app=fluentd` | 增加 memory limits |
| 输出失败 | 后端不可达 | `kubectl logs -l app=fluentd -n logging` | 检查 Output URL 和网络 |
| 日志延迟高 | 缓冲区积压 | `kubectl exec fluentd-0 -- du -sh /buffers/` | 增加 flush 频率或后端容量 |
| Fluent Bit 重启 | 配置错误 | `kubectl logs -l app=fluent-bit -n logging --previous` | 检查 Logging CRD 配置 |

```
排查流程：
├── 日志未到达后端？
│   ├── kubectl get flows → 确认 Flow 存在且匹配
│   ├── kubectl logs fluentd → 查看处理错误
│   └── 检查 Output 连接配置
├── 日志丢失？
│   ├── 检查缓冲区是否溢出
│   ├── 确认 PVC 持久化缓冲已配置
│   └── 检查 Fluent Bit 采集状态
└── 性能问题？
    ├── 检查 Fluentd CPU/内存使用
    ├── 调整 buffer flush_interval
    └── 考虑增加 Fluentd 副本
```

## 生产案例

### 案例 1：多租户日志隔离

- **场景**：SaaS 平台 50+ 租户，每个租户的日志需要发送到独立的 Elasticsearch 索引
- **排查**：手动为每个命名空间配置 Fluentd 配置文件，维护成本极高
- **方案**：每个租户命名空间创建独立的 Flow + Output CRD，自动路由到对应 ES 索引
- **效果**：新租户日志配置从 2h 缩短至 5min，零手动 Fluentd 配置

### 案例 2：日志管道缓冲保护

- **场景**：Elasticsearch 维护期间 2h 不可用，恢复后大量日志丢失
- **排查**：Fluentd 使用内存缓冲，ES 不可用时缓冲溢出导致日志丢弃
- **方案**：配置 PVC 持久化缓冲区（20Gi），设置 retry_max_interval=300s，ES 恢复后自动重发
- **效果**：后续 ES 维护期间零日志丢失，缓冲自动消化

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Logging Operator** | CRD 声明式、双层架构 | 配置复杂度高 |
| Promtail + Loki | Grafana 原生集成 | 功能单一（仅 Loki 输出） |
| Vector | Rust 高性能、统一日志+指标 | 非 K8s 原生配置 |
| Filebeat | ELK 生态标准 | 配置手动、无 CRD |

## 架构定位

在 CNCF 生态中，Logging Operator 属于 **Observability / Logging** 类别，是 Kubernetes 日志管道的声明式管理方案。它与 Loki、Elasticsearch、Grafana 等项目协同工作。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[opengemini]] — openGemini
- [[kmesh]] — Kmesh
- [[kpt]] — kpt
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[fluentd]] — Fluentd

- logging-operator
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
