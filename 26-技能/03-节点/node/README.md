---
title: Node 异常诊断技能集
description: Node 异常状态（NotReady、MemoryPressure、DiskPressure、PIDPressure、Unknown 等）的完整诊断技能体系，包含 FTA 故障树、SOP 操作流程、命令输出解读及生产案例
summary: Node 异常诊断技能集入口，涵盖节点不可用、资源压力、组件故障、批量级联等全场景故障诊断
category: skill
tags:
- k8s
- node
- troubleshooting
- fta
- sop
- runbook
- notready
- kubelet
- containerd
- diskpressure
- memorypressure
- version-diff
- compatibility
sources:
- 故障诊断/技能体系/01-node-notready.md
- 故障诊断/技能体系/19-node-resource-pressure.md
- 故障诊断/FTA故障树/list/node-fta.md
- 故障诊断/FTA故障树/list/nodepool-fta.md
- 故障诊断/核心排障/06-node-notready-diagnosis.md
- 故障诊断/高级排障/35-node-component-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/04-node-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/06-gpu-device-plugin-troubleshooting.md
- 故障诊断/技能体系/skill-set/k8s-node-notready/
- 技能/troubleshoot-node-issues.md
- 技能/ts-node-components.md
- code/kubernetes-release-1.18/
- code/kubernetes-release-1.28/
- code/kubernetes-release-1.30/
- code/kubernetes-release-1.32/
- code/kubernetes-release-1.34/
- code/kubernetes-1.36.2/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Node 异常诊断技能有哪些
- 节点故障排查从哪里开始
- Node NotReady 怎么排查
- 节点资源压力如何解决
- kubelet 异常怎么诊断
trigger_keywords:
- Node
- NotReady
- MemoryPressure
- DiskPressure
- PIDPressure
- 节点异常
- 节点不可用
- kubelet
- containerd
- 节点故障
prerequisites:
- kubectl-basics
- node-architecture
- troubleshooting-methodology
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Node 异常诊断技能集

## 概述

本技能集整合了 Kubernetes Node 全生命周期异常诊断的完整知识体系，覆盖从节点不可用到资源压力、组件故障的所有常见故障场景。内容来源于生产工单实践、FTA 故障树分析及标准化 SOP 流程。

**适用场景**：
- 节点状态异常（NotReady / Unknown / SchedulingDisabled）
- 节点资源压力（MemoryPressure / DiskPressure / PIDPressure）
- 节点组件故障（kubelet / containerd / kube-proxy / CNI）
- GPU/设备插件故障（NVIDIA / MIG / RDMA）
- 批量节点级联故障
- 证书过期与认证失败
- AI Agent 自动化诊断执行

---

## 技能文件索引

| # | 文件 | 覆盖场景 | 难度 | 预计阅读 |
|---|------|---------|------|---------|
| 01 | [01-node-notready-diagnosis.md](01-node-notready-diagnosis.md) | NotReady、Unknown、kubelet 崩溃、网络分区、证书过期 | 高级 | 30min |
| 02 | [02-node-resource-pressure.md](02-node-resource-pressure.md) | MemoryPressure、DiskPressure、PIDPressure、Pod 驱逐 | 高级 | 25min |
| 03 | [03-node-component-troubleshooting.md](03-node-component-troubleshooting.md) | kubelet/containerd/kube-proxy 组件级故障排查 | 高级 | 20min |
| 04 | [04-node-sop-runbook.md](04-node-sop-runbook.md) | 标准操作流程（SOP）、Runbook、升级决策、自动化诊断、ACK 云环境 | 中级 | 20min |
| 05 | [05-gpu-device-plugin-troubleshooting.md](05-gpu-device-plugin-troubleshooting.md) | GPU/设备插件故障、CUDA 兼容、MIG、RDMA、DRA | 高级 | 20min |

### 参考资料

| 文件 | 说明 |
|------|------|
| [reference/node-conditions-reference.md](reference/node-conditions-reference.md) | Node Conditions 完整参考与诊断映射 |
| [reference/node-root-cause-catalog.md](reference/node-root-cause-catalog.md) | 根因目录（15 种）与修复方案速查表（含详细 RC 描述与交叉关联图） |
| [reference/node-version-differences.md](reference/node-version-differences.md) | Kubernetes 版本差异对比与兼容性矩阵（基于源码分析，含诊断命令差异与误诊模式） |
| [reference/nodepool-fta.md](reference/nodepool-fta.md) | NodePool 异常故障树分析（扩缩容、初始化、网络、配额等 7 大类） |

---

## 快速诊断入口

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看异常节点概览
kubectl get nodes -o wide | grep -v " Ready"

# Step 2: 查看特定节点详情
kubectl describe node <node-name> | grep -A 10 "Conditions:"

# Step 3: 检查节点事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
  --sort-by=.lastTimestamp --no-headers | tail -20

# Step 4: 检查节点 Lease 续租
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
```

---

## Node 状态速查表

| 状态 | 含义 | 常见原因 | 优先检查项 | 对应技能 |
|:---|:---|:---|:---|:---|
| **NotReady** | 节点异常 | kubelet 崩溃/网络分区/资源压力 | kubelet 状态 + 日志 | 01 |
| **Unknown** | 状态未知 | 节点失联/apiserver 连接断开 | 网络连通性 | 01 |
| **SchedulingDisabled** | 禁止调度 | 手动 cordon/维护 | 检查是否手动操作 | 04 |
| **MemoryPressure** | 内存压力 | 内存不足/内存泄漏 | 节点内存使用 | 02 |
| **DiskPressure** | 磁盘压力 | 磁盘空间/inode 不足 | 节点磁盘使用 | 02 |
| **PIDPressure** | PID 压力 | 进程数过多/线程泄漏 | 节点 PID 使用 | 02 |
| **NetworkUnavailable** | 网络不可用 | CNI 未配置/异常 | CNI Pod 状态 | 03 |

---

## FTA 故障树路径映射

| 顶层事件 | 中间事件 | 底事件 | 对应技能 |
|---------|---------|--------|---------|
| TE-1 节点不可用 | IE-1.1 kubelet 异常 | BE-1.1 kubelet 崩溃 | 01 |
| TE-1 节点不可用 | IE-1.2 容器运行时异常 | BE-1.2 containerd 停止 | 01 |
| TE-1 节点不可用 | IE-1.3 网络分区 | BE-1.3 节点-apiserver 不通 | 01 |
| TE-1 节点不可用 | IE-1.4 资源压力 | BE-1.4~1.6 磁盘/内存/PID | 02 |
| TE-1 节点不可用 | IE-1.5 证书过期 | BE-1.7 TLS 握手失败 | 01 |
| TE-1 节点不可用 | IE-1.6 硬件/内核 | BE-1.8 内核 panic | 01 |

---

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[19-故障诊断/06-FTA故障树/list/node-fta.md|Node 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/nodepool-fta.md|NodePool 异常故障树分析]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/SKILL.md|Node NotReady Skill]]
- [[26-技能/03-节点/node/诊断排障/troubleshoot-node-issues.md|节点故障排查基础]]
- [[26-技能/03-节点/node/诊断排障/ts-node-components.md|节点组件结构化排障]]

## Related

- [[kubelet]] — kubelet 节点代理
- [[containerd]] — 容器运行时
- [[kube-proxy]] — 网络代理
