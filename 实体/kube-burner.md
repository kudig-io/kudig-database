---
title: Kube-burner (entities)
description: '## 概述'
summary: 'Kube-burner 是一个 Kubernetes 性能和规模测试工具，通过在集群中创建或删除大量对象来模拟各种负载场景，并收集详细的性能指标。它广泛用于 Kubernetes 发行版（如 OpenShift）的可扩展性测试和基准测试。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kube-burner
- etcd
- prometheus
- grafana
- cilium
- elasticsearch
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kube-burner 是什么
- 如何 Kube-burner
trigger_keywords:
- Kube-burner
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kube-burner

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Kube-burner 是由 Red Hat（OpenShift 性能工程团队）开发的 Kubernetes 性能和规模基准测试工具，2023 年进入 CNCF Sandbox。它通过在集群中批量创建或删除大量 Kubernetes 对象（Pod、Deployment、Namespace、Secret 等），模拟各种规模和负载场景，同时收集详细的性能指标（API 延迟、etcd 性能、调度延迟等）。

Kube-burner 广泛用于 Kubernetes 发行版的可扩展性验证、SLO 基准测试和容量规划。OpenShift 团队使用它来验证每个版本在 5000+ 节点规模下的性能表现。它支持通过 YAML 配置文件定义测试场景（工作负载模板、迭代次数、间隔），并自动将 Prometheus 指标索引到 Elasticsearch 供趋势分析。

## Key Features

- **大规模对象操作**：快速创建/删除数万个 Kubernetes 对象（Pod、Service、Deployment 等）
- **工作负载模板**：通过 YAML 定义工作负载（模板、副本数、命名空间数）
- **指标采集**：自动从 Prometheus 抓取 API 延迟、调度延迟、etcd 性能等指标
- **Elasticsearch 索引**：将测试结果索引到 ES，支持历史趋势和回归分析
- **告警阈值**：支持定义 SLO 阈值，超限时自动标记测试为失败
- **多场景支持**：内置 node-density、cluster-density、pod-density 等标准基准场景

## Architecture

Kube-burner 由 **CLI 工具**（命令行执行器）、**Workload Config**（YAML 定义工作负载模板和参数）、**Metrics Collector**（从 Prometheus 查询指标）和 **Indexer**（将结果写入 Elasticsearch/本地 JSON）组成。工作负载执行时，Kube-burner 使用多 goroutine 并发调用 Kubernetes API 创建对象，同时计时每个 API 调用的延迟。测试完成后汇总 P50/P95/P99 延迟和错误率。

## K8s 集成

Kube-burner 作为标准 Kubernetes 客户端工具运行，通过 kubeconfig 连接到目标集群的 API Server。它不修改集群安装——只是一个客户端工具，创建和删除标准 K8s 资源。需要在集群中预装 Prometheus（用于指标采集）和可选的 Elasticsearch（用于结果存储）。

## 生产部署要点

- **基线测试**：先在空集群运行获取基线数据，再对比优化后的结果
- **渐进负载**：从低 QPS 开始逐步提高，找到集群的吞吐瓶颈
- **指标存储**：使用 Elasticsearch 持久化结果，便于趋势分析和回归检测
- **告警阈值**：根据 SLO 设定合理的告警阈值，及时发现性能回退
- **资源隔离**：在专用测试集群运行，避免影响生产环境
- **重复执行**：每次测试多次运行取平均值，减少偶发因素影响

## 生产场景

1. **集群容量规划**：验证集群能承载多少 Pod/Service，找到性能瓶颈
2. **版本升级基准**：K8s 升级前后对比 API 延迟和调度性能
3. **SLO 验证**：验证 Pod 创建延迟是否满足 P99 < 5s 的 SLO
4. **CNI/CSI 性能对比**：不同网络/存储插件的性能基准测试

## 安装与配置

### CLI 安装

```bash
# 安装 Kube-burner CLI
wget https://github.com/cloud-bulldozer/kube-burner/releases/latest/download/kube-burner-$(uname)-x86_64.tar.gz
tar xzf kube-burner-*.tar.gz && sudo mv kube-burner /usr/local/bin/

# 验证安装
kube-burner version
```

### 内置工作负载测试

```bash
# 节点密度测试（每节点创建 250 个 Pod）
kube-burner init --metrics-endpoint=http://prometheus:9090 \
  --es-endpoint=https://elasticsearch:9200 \
  --workload=node-density \
  --pods-per-node=250

# 集群密度测试
kube-burner init --workload=cluster-density \
  --namespaces=10 \
  --iterations=100

# API 压力测试
kube-burner init --workload=api-intensive \
  --qps=20 --burst=20
```

### 自定义工作负载

```yaml
# my-workload.yml
name: stress-test
global:
  gc: true
  gcMetrics: true
  measurements:
    - name: podLatency
      esIndex: kube-burner
jobs:
  - name: create-deployments
    jobType: create
    jobIterations: 10
    qps: 5
    burst: 10
    namespacedIterations: true
    namespace: stress-test
    objects:
      - objectTemplate: deployment-template.yml
        replicas: 50
        inputVars:
          cpuRequest: 100m
          memRequest: 128Mi
  - name: delete-all
    jobType: delete
    objects:
      - kind: Namespace
        labelSelector: {kube-burner-job: create-deployments}
```

```bash
# 执行自定义工作负载
kube-burner init --workload=my-workload.yml \
  --metrics-endpoint=http://prometheus:9090
```

## 运维操作

```bash
# 🟢 查看测试结果指标
kube-burner init --workload=node-density --dry-run

# 🟡 执行压力测试（会创建大量资源）
kube-burner init --workload=node-density --pods-per-node=100

# 🟡 指定 Prometheus 指标采集
kube-burner init --workload=cluster-density \
  --metrics-endpoint=http://prometheus:9090 \
  --metrics-profile=metrics-aggregated.yml

# 🔴 清理测试资源（gc: true 自动清理）
kube-burner destroy --workload=my-workload.yml

# 🔴 删除测试创建的命名空间
kubectl delete ns -l kube-burner-uuid=<uuid>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 测试超时 | 集群资源不足 | `kubectl get events --sort-by=.lastTimestamp` | 降低 iterations 或增加节点 |
| Pod 大量 Pending | 节点资源耗尽 | `kubectl describe node \| grep -A5 Allocated` | 减少 pods-per-node |
| Prometheus 指标丢失 | 指标端点不可达 | `curl http://prometheus:9090/-/healthy` | 检查 Prometheus 服务和网络 |
| etcd 延迟飙升 | API 请求过多 | `etcdctl endpoint status` | 降低 QPS/Burst 参数 |
| 测试结果不准确 | 节点负载不均 | `kubectl top nodes` | 使用 nodeSelector 固定测试节点 |

**排查流程：**
```
压力测试异常
├── 检查集群健康 → kubectl get nodes && kubectl get cs
├── 检查资源余量 → kubectl top nodes
├── 检查 API Server 延迟 → kubectl get --raw /healthz?verbose
├── 检查 etcd 状态 → etcdctl endpoint health
└── 检查测试配置 → 确认 QPS/Burst/iterations 合理
```

## 生产案例

### 案例一：集群容量规划

- **场景**: 新集群上线前需验证能承载 5000 个 Pod 的目标容量
- **排查**: 使用 kube-burner node-density 逐步增加每节点 Pod 数，观察调度延迟和 API 响应
- **方案**: 从 100 Pod/节点开始，每次增加 50，记录 P99 调度延迟，找到拐点
- **效果**: 确定最优密度为 250 Pod/节点，超过后 P99 延迟超过 SLO（5s）

### 案例二：K8s 版本升级基准对比

- **场景**: 从 K8s 1.28 升级到 1.30，需验证性能无回退
- **排查**: 升级前后分别运行相同 kube-burner 工作负载，对比指标
- **方案**: 使用 cluster-density 工作负载，对比 Pod 创建 P50/P95/P99 延迟、API 吐吐量
- **效果**: 确认 1.30 调度性能提升 12%，无回退，安全升级

## 对比

| 特性 | Kube-burner | k6 | Kwok | kind | 适用场景 |
|------|------------|-----|------|------|----------|
| K8s 对象压力 | ✅ | ⚠️ | ✅ | ❌ | kube-burner 专业 |
| 指标采集 | ✅ Prometheus | ✅ | ❌ | ❌ | - |
| 模拟节点 | ❌ | ❌ | ✅ | ❌ | Kwok 大规模模拟 |
| 结果存储 | ✅ ES | ✅ | ❌ | ❌ | - |
| 自定义工作负载 | ✅ YAML | ✅ JS | ⚠️ | ❌ | - |

## 参考链接

- [[etcd]]
- [[实体/prometheus-grafana.md|prometheus-grafana]]
- networking.md|cilium-ebpf-networking]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[deployment]]

## Related

- [[slimtoolkit]] — SlimToolkit
- [[cni]] — CNI (Container Network Interface)
- [[实体/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[external-secrets]] — External Secrets Operator
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-burner
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
