---
title: Kured (KUbernetes REboot Daemon)
description: '## 概述'
summary: 'Kured (KUbernetes REboot Daemon) 是一个 Kubernetes 守护进程，用于在节点需要重启时安全地执行重启操作。它检测节点上的重启信号 (如 /var/run/reboot-required 文件)，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐工作负载。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kured
- prometheus
- grafana
- coredns
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kured (KUbernetes REboot Daemon) 是什么
- 如何 Kured (KUbernetes REboot Daemon)
trigger_keywords:
- Kured
- KUbernetes
- REboot
- Daemon
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kured|Kured]] (KUbernetes REboot Daemon)

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Kured（KUbernetes REboot Daemon）是由 Weaveworks 开发的 Kubernetes 节点重启守护进程，2020 年加入 CNCF Sandbox。它在节点需要重启时（如内核安全更新）安全地执行重启操作，检测 `/var/run/reboot-required` 信号，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐（Drain）工作负载。Kured 使 Kubernetes 节点的安全补丁管理变得自动化和安全。

## 核心特性

- **自动检测**: 检测 OS 发出的重启信号（reboot-required 文件）
- **协调重启**: 通过 Kubernetes Lease 确保一次只重启一个节点
- **Cordon/Drain**: 重启前自动 cordon 节点并 drain Pod
- **时间窗口**: 支持配置允许重启的时间窗口（如夜间低峰）
- **Prometheus 指标**: 暴露待重启节点数等监控指标
- **通知集成**: 支持 Slack、Teams、Webhook 通知

## 架构

Kured 以 DaemonSet 形式部署在每个节点上。每个 Kured Pod 作为特权容器运行，挂载主机的文件系统以检测 `/var/run/reboot-required` 文件。当检测到重启信号时，Kured 尝试获取集群级的 Kubernetes Lease（作为分布式锁）。获取到锁的节点执行：cordon → drain → reboot → uncordon 流程。其他节点等待锁释放后再依次执行。这确保了同一时间只有一个节点在重启，避免服务中断。

## Kubernetes 集成

Kured 通过 DaemonSet 运行在所有节点上，通过 Kubernetes API 获取和释放分布式锁（Lease 对象）。重启流程利用 `kubectl cordon` 和 `kubectl drain` 原语隔离节点并驱逐 Pod。尊重 PodDisruptionBudget，不会强制驱逐受保护的 Pod。通过 Prometheus 指标（`kured_reboot_required`）暴露状态，支持 AlertManager 集成告警。

## 生产使用场景

1. **安全补丁自动化**: 节点 OS 安装内核安全更新后，Kured 自动安全重启
2. **合规要求**: 满足定期重启/补丁管理的合规审计要求
3. **维护窗口**: 配置低峰期重启窗口，最小化业务影响
4. **大规模集群**: 协调数百个节点的滚动重启

## 安装与配置

```bash
# Helm 安装
helm repo add kubereboot https://kubereboot.github.io/charts/
helm install kured kubereboot/kured \
  --namespace kured --create-namespace \
  --set configuration.startTime="0:00" \
  --set configuration.endTime="4:00" \
  --set configuration.timeZone="Asia/Shanghai" \
  --set configuration.rebootDays="sat,sun" \
  --set slack.enabled=true \
  --set slack.channel="#alerts" \
  --set slack.username="kured"

# 或一键安装
kubectl apply -f https://github.com/kubereboot/kured/releases/download/1.16.2/kured-1.16.2-dockerhub.yaml

# 验证安装
kubectl get pods -n kured
kubectl get daemonset kured -n kured
```

```yaml
# Kured Helm values 自定义
configuration:
  startTime: "2:00"      # 允许重启开始时间
  endTime: "5:00"        # 允许重启结束时间
  timeZone: "Asia/Shanghai"
  rebootDays: "sat,sun"  # 仅周末重启
  period: "1h"           # 检查间隔
  drainTimeout: "5m"     # Drain 超时
  drainGracePeriod: "30" # Pod 优雅终止时间
  skipWaitForDeleteTimeout: "60"
  forceReboot: false     # 是否强制重启 (drain 失败时)
  rebootCommand: "/usr/bin/systemctl reboot"
  annotateNodes: true    # 重启时添加节点 annotation
  lockAnnotation: "weave.works/kured-node-lock"

# Prometheus 指标
metrics:
  create: true
  namespace: monitoring

# Slack 通知
slack:
  enabled: true
  channel: "#ops-alerts"
  username: "kured"
```

## 运维操作

```bash
# 🟢 检查 Kured 状态
kubectl get pods -n kured -o wide
kubectl get daemonset kured -n kured

# 🟢 检查待重启节点
kubectl get nodes -l weave.works/kured-reboot-in-progress=true
kubectl get nodes -o custom-columns=NAME:.metadata.name,REBOOT:.metadata.annotations.'weave\.works/kured-reboot-in-progress'

# 🟢 检查 Kured 日志
kubectl logs -n kured -l app=kured --tail=30

# 🟢 检查分布式锁状态
kubectl get lease -n kured kured-lock -o yaml 2>/dev/null || \
  kubectl get configmap -n kured kured-lock -o yaml

# 🟢 检查 Prometheus 指标
curl -s http://<kured-pod-ip>:8080/metrics | grep kured
# kured_reboot_required{node="node-1"} 1

# 🟡 手动阻止重启 (维护模式)
kubectl annotate node <node> weave.works/kured-reboot-blocked=true

# 🟡 解除重启阻止
kubectl annotate node <node> weave.works/kured-reboot-blocked-

# 🟡 手动触发节点重启 (通过 Kured)
kubectl annotate node <node> weave.works/kured-most-recent-reboot-needed=true

# 🔴 强制重启节点 (跳过 Kured 协调)
ssh <node> sudo systemctl reboot
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 节点未自动重启 | 不在重启时间窗口 | 检查 Kured 日志 | 调整 startTime/endTime |
| Drain 超时 | PDB 阻止驱逐 | `kubectl get pdb -A` | 调整 PDB 或 drainTimeout |
| 锁未释放 | 上次重启异常 | 检查 Lease/ConfigMap | 手动删除锁对象 |
| 多节点同时重启 | 锁机制失效 | 检查 Kured 日志 | 检查 RBAC 权限 |
| 重启后 Pod 未恢复 | Uncordon 失败 | `kubectl get nodes` | 手动 `kubectl uncordon` |
| 指标不可用 | metrics 未启用 | 检查 Helm values | 启用 metrics.create |

### 排查流程

```
Kured 重启异常
├── 节点未重启
│   ├── 检查 /var/run/reboot-required 是否存在
│   ├── 检查 Kured 日志确认检测状态
│   ├── 检查时间窗口配置 (startTime/endTime/rebootDays)
│   └── 检查是否被 annotate 阻止
├── Drain 失败
│   ├── kubectl get pdb -A → 检查 PDB
│   ├── kubectl get pods --field-selector spec.nodeName=<node>
│   ├── 检查 DaemonSet Pod (不可驱逐)
│   └── 调整 drainTimeout 或 forceReboot
└── 锁机制异常
    ├── 检查 Lease/ConfigMap 状态
    ├── 检查 RBAC 权限
    └── 手动删除锁对象恢复
```

## 生产案例

### 案例 1: 大规模集群安全补丁自动化

- **场景**: 200 节点集群，每周需安装内核安全更新并重启
- **排查**: 手动重启耗时且容易遗漏；曾发生同时重启多节点导致服务中断
- **方案**: 部署 Kured；配置周末凌晨 2-5 点重启窗口；配合 PDB 保护关键服务；Slack 通知重启状态
- **效果**: 补丁管理完全自动化；零服务中断；合规审计通过

### 案例 2: Drain 超时导致重启延迟

- **场景**: 某节点持续报 reboot-required 但未重启
- **排查**: Kured 日志显示 drain 超时；原因是 StatefulSet Pod 的 PDB minAvailable=3 但只有 3 副本
- **方案**: 调整 PDB 为 maxUnavailable=1；增加 drainTimeout 到 10m；配置 forceReboot=false 避免数据丢失
- **效果**: Drain 成功完成；节点正常重启；数据完整

## 对比与替代方案

| 维度 | Kured | Talos Linux | Flatcar + FLUO | AWS SSM |
|------|-------|-------------|----------------|----------|
| 重启协调 | ✅ Lease 锁 | ✅ 内置 | ✅ locksmith | ✅ 维护窗口 |
| 自动更新 | ❌ 仅重启 | ✅ 不可变 OS | ✅ A/B 分区 | ✅ Patch Manager |
| K8s 原生 | ✅ | ✅ | 部分 | ❌ |
| 复杂度 | 低 | 高 (需替换 OS) | 中 | 中 |
| 适用场景 | 通用节点重启 | 全新集群 | CoreOS 用户 | AWS 环境 |

## 检查清单

- [ ] Kured DaemonSet 所有 Pod Running
- [ ] 重启时间窗口已配置 (避免业务高峰)
- [ ] PDB 已配置保护关键服务
- [ ] Prometheus 指标已接入监控
- [ ] 告警规则已配置 (待重启节点数)
- [ ] Slack/通知渠道已配置
- [ ] RBAC 权限正确 (Lease/Node 操作)
- [ ] 定期验证重启流程 (测试环境)

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[pod-lifecycle]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]

## Related

- [[notary-project]] — Notary Project
- [[coredns]] — CoreDNS
- [[contour]] — Contour
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/03-运行时/flatcar.md|flatcar]] — Flatcar Container Linux

<!-- risk-assessed -->
