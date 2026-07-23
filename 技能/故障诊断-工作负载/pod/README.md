---
title: Pod 技能集（诊断 · 概念 · 调度 · 安全 · 运维全景）
description: Kubernetes Pod 全主题技能集入口，整合 Pod 异常诊断、生命周期、调度、资源与自动扩缩、安全、清单规范、配置字典、工单案例、培训、云厂商与上游源码文档，构成完整的 Pod 知识与排障体系
summary: Pod 技能集 MOC 入口，按功能模块组织全项目 Pod 相关内容，涵盖诊断排障、概念原理、调度、资源扩缩、安全、清单规范、配置字典、生命周期、工单案例、培训、云厂商、生态索引及上游源码
category: skill
tags:
- k8s
- pod
- troubleshooting
- fta
- sop
- runbook
- lifecycle
- scheduling
- security
- autoscaling
- moc
sources:
- 故障诊断/topic-skills/02-pod-crashloop-oomkilled.md
- 故障诊断/topic-skills/03-pod-pending.md
- 故障诊断/FTA故障树/list/pod-fta.md
- 故障诊断/核心排障/08-pod-comprehensive-troubleshooting.md
- 系统基础/知识字典/workloads/pod.md
- 工作负载/pods/pod-lifecycle.md
- 最佳实践/security/pod-security.md
- 源码/kruise-1.9.1/docs/proposals/
- code/kubernetes-1.36.2/pkg/apis/core/types.go
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
- 平台工程师
- 所有工程师
estimated_read_time: 8min
intent_queries:
- Pod 技能有哪些
- Pod 故障排查从哪里开始
- CrashLoopBackOff 怎么排查
- Pod Pending 如何解决
- Pod 调度机制是什么
- Pod 安全标准如何配置
- Pod 资源与自动扩缩怎么做
trigger_keywords:
- Pod
- CrashLoopBackOff
- OOMKilled
- Pending
- ImagePullBackOff
- 容器崩溃
- 调度失败
- Pod 安全
- Pod 生命周期
- HPA
- VPA
prerequisites:
- kubectl-basics
- pod-lifecycle
---

> **生产环境安全提示**
>
> 本技能集包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod 技能集

## 概述

本技能集整合了 KUDIG-DATABASE 项目中**全部 Pod 相关内容**，按功能主题组织成统一的知识与排障体系。内容来源于诊断技能、FTA 故障树、生产工单、知识字典、上游 Kubernetes/Kruise 源码文档、培训材料及云厂商实践，覆盖 Pod 从概念、调度、生命周期到异常诊断、安全加固、资源治理的全景。

**适用场景**：
- Pod 状态异常诊断（CrashLoopBackOff / Pending / OOMKilled / ImagePullBackOff / Evicted / Terminating）
- Pod 概念原理与生命周期理解
- Pod 调度机制（亲和性、拓扑分布、优先级抢占、调度就绪）
- Pod 资源管理与自动扩缩（QoS、HPA、VPA、原地扩缩）
- Pod 安全（PSS/PSA、安全上下文、内核约束）
- Pod 清单编写与规范
- AI Agent 自动化诊断执行
- 教学培训与知识索引

---

## 核心诊断技能（快速入口）

按诊断优先级排列，是本技能集最常用的四篇 SOP 化诊断文档：

| # | 文件 | 覆盖场景 | 难度 | 预计阅读 |
|---|------|---------|------|---------|
| 01 | [01-pod-crashloop-oomkilled.md](01-pod-crashloop-oomkilled.md) | CrashLoopBackOff、OOMKilled、Exit Code 分析 | 高级 | 25min |
| 02 | [02-pod-pending-scheduling.md](02-pod-pending-scheduling.md) | Pending、调度失败、资源不足、污点/亲和性 | 中级 | 20min |
| 03 | [03-pod-imagepull-container.md](03-pod-imagepull-container.md) | ImagePullBackOff、容器创建失败、Init 容器错误 | 中级 | 15min |
| 04 | [04-pod-sop-runbook.md](04-pod-sop-runbook.md) | 标准操作流程（SOP）、Runbook、升级决策 | 中级 | 15min |
| — | [pod-fta.md](pod-fta.md) | Pod 异常故障树分析（FTA） | 高级 | 20min |

### 参考资料 · reference/

| 文件 | 说明 |
|------|------|
| [reference/pod-exit-codes.md](reference/pod-exit-codes.md) | 容器退出码完整参考与诊断映射 |
| [reference/pod-version-differences.md](reference/pod-version-differences.md) | Pod 诊断的 Kubernetes 版本差异与兼容性矩阵（源码实证） |

---

## 功能模块索引

### 🔍 诊断排障 · [诊断排障/](诊断排障/)

多来源的 Pod 排障文档、结构化排障流程、视频脚本及真实案例复盘。

| 文件 | 说明 |
|------|------|
| [诊断排障/08-pod-comprehensive-troubleshooting.md](诊断排障/08-pod-comprehensive-troubleshooting.md) | Pod 综合排障（核心排障篇） |
| [诊断排障/05-pod-pending-diagnosis.md](诊断排障/05-pod-pending-diagnosis.md) | Pending 诊断 |
| [诊断排障/troubleshoot-pod-issues.md](诊断排障/troubleshoot-pod-issues.md) | Pod 问题排查（上游文档） |
| [诊断排障/structural-01-pod-troubleshooting.md](诊断排障/structural-01-pod-troubleshooting.md) | 结构化 Pod 排障流程 |
| [诊断排障/MULTI-002-Pod-CrashLoop-镜像拉取失败并发.md](诊断排障/MULTI-002-Pod-CrashLoop-镜像拉取失败并发.md) | 多故障并发场景 |
| [诊断排障/pod-fta-故障树list.md](诊断排障/pod-fta-故障树list.md) | FTA 故障树清单 |
| [诊断排障/video-script-pod-crashloop.md](诊断排障/video-script-pod-crashloop.md) | CrashLoop 教学视频脚本 |
| [诊断排障/技能体系-02-pod-crashloop-oomkilled.md](诊断排障/技能体系-02-pod-crashloop-oomkilled.md) | 技能体系原始篇：CrashLoop/OOMKilled |
| [诊断排障/技能体系-03-pod-pending.md](诊断排障/技能体系-03-pod-pending.md) | 技能体系原始篇：Pending |
| [诊断排障/案例/](诊断排障/案例/) | 4 篇真实故障案例复盘（节点驱逐、docker 僵死、污点误配、PDB 阻塞排空） |

### 📚 概念原理 · [概念原理/](概念原理/)

Pod 定义、生命周期、架构关系及与其他组件的交叉主题。

| 文件 | 说明 |
|------|------|
| [概念原理/pods.md](概念原理/pods.md) | Pod 上游概念文档 |
| [概念原理/pod-lifecycle.md](概念原理/pod-lifecycle.md) | Pod 生命周期（上游） |
| [概念原理/architecture-pod-lifecycle.md](概念原理/architecture-pod-lifecycle.md) | 架构视角的生命周期 |
| [概念原理/containerd-pod-lifecycle.md](概念原理/containerd-pod-lifecycle.md) | containerd 视角的生命周期 |
| [概念原理/pod-availability-lifecycle.md](概念原理/pod-availability-lifecycle.md) | 可用性与生命周期 |
| [概念原理/pod-affinity.md](概念原理/pod-affinity.md) | Pod 亲和性概念 |
| [概念原理/pod-overhead.md](概念原理/pod-overhead.md) | Pod 开销概念 |
| [概念原理/apiserver-×-Pod诊断.md](概念原理/apiserver-×-Pod诊断.md) | API Server × Pod 诊断 |
| [概念原理/etcd-×-Pod诊断.md](概念原理/etcd-×-Pod诊断.md) | etcd × Pod 诊断 |
| [概念原理/Operator模式×Pod生命周期.md](概念原理/Operator模式×Pod生命周期.md) | Operator × 生命周期 |
| [概念原理/Pod生命周期×Secret管理.md](概念原理/Pod生命周期×Secret管理.md) | 生命周期 × Secret |
| [概念原理/Pod生命周期×存储模型.md](概念原理/Pod生命周期×存储模型.md) | 生命周期 × 存储 |
| [概念原理/字典-pod.md](概念原理/字典-pod.md)、[字典-pods.md](概念原理/字典-pods.md)、[字典-pod-lifecycle.md](概念原理/字典-pod-lifecycle.md) | 知识字典条目 |

### 🔄 生命周期与事件 · [生命周期与事件/](生命周期与事件/)

| 文件 | 说明 |
|------|------|
| [生命周期与事件/02-pod-container-lifecycle-events.md](生命周期与事件/02-pod-container-lifecycle-events.md) | Pod 与容器生命周期事件 |
| [生命周期与事件/11-pod-lifecycle-events.md](生命周期与事件/11-pod-lifecycle-events.md) | 生命周期事件详解 |

### 🗓️ 调度 · [调度/](调度/)

| 文件 | 说明 |
|------|------|
| [调度/assigning-pods-to-nodes.md](调度/assigning-pods-to-nodes.md) | 将 Pod 分配到节点 |
| [调度/pod-topology-spread-constraints.md](调度/pod-topology-spread-constraints.md) | 拓扑分布约束 |
| [调度/pod-priority-and-preemption.md](调度/pod-priority-and-preemption.md) | 优先级与抢占 |
| [调度/pod-scheduling-readiness.md](调度/pod-scheduling-readiness.md) | 调度就绪（SchedulingGates） |
| [调度/01-gpu-pod-scheduling.md](调度/01-gpu-pod-scheduling.md) | GPU Pod 调度 |
| [调度/字典-pod-overhead.md](调度/字典-pod-overhead.md) | Pod Overhead 字典 |

### 📈 资源与自动扩缩 · [资源与自动扩缩/](资源与自动扩缩/)

| 文件 | 说明 |
|------|------|
| [资源与自动扩缩/resource-management-for-pods-and-containers.md](资源与自动扩缩/resource-management-for-pods-and-containers.md) | Pod/容器资源管理 |
| [资源与自动扩缩/pod-quality-of-service-classes.md](资源与自动扩缩/pod-quality-of-service-classes.md) | QoS 分级 |
| [资源与自动扩缩/horizontal-pod-autoscaler.md](资源与自动扩缩/horizontal-pod-autoscaler.md) | HPA 水平扩缩 |
| [资源与自动扩缩/vertical-pod-autoscaling.md](资源与自动扩缩/vertical-pod-autoscaling.md) | VPA 垂直扩缩 |
| [资源与自动扩缩/29-in-place-pod-resize.md](资源与自动扩缩/29-in-place-pod-resize.md) | 原地扩缩（In-Place Resize） |
| [资源与自动扩缩/字典-horizontal-pod-autoscaling.md](资源与自动扩缩/字典-horizontal-pod-autoscaling.md) | HPA 字典 |

### 🔐 安全 · [安全/](安全/)

Pod 安全标准（PSS）、安全准入（PSA）、安全策略（PSP）、内核约束及排障。

| 文件 | 说明 |
|------|------|
| [安全/01-pod-security-standards-reference.md](安全/01-pod-security-standards-reference.md) | PSS 标准参考 |
| [安全/06-pod-security-standards.md](安全/06-pod-security-standards.md) | PSS 标准 |
| [安全/02-pod-security-admission-deep-dive.md](安全/02-pod-security-admission-deep-dive.md) | PSA 准入深入 |
| [安全/15-pod-security-standards-migration.md](安全/15-pod-security-standards-migration.md) | PSP→PSA 迁移 |
| [安全/k8s-pod-security-guide.md](安全/k8s-pod-security-guide.md) | Pod 安全指南 |
| [安全/best-practices-pod-security.md](安全/best-practices-pod-security.md)、[安全/security-pod-security.md](安全/security-pod-security.md) | 安全最佳实践（两来源） |
| [安全/linux-kernel-security-constraints-for-pods-and-containers.md](安全/linux-kernel-security-constraints-for-pods-and-containers.md) | 内核安全约束 |
| [安全/structural-03-pod-security-troubleshooting.md](安全/structural-03-pod-security-troubleshooting.md) | 安全排障 |
| [安全/概念-pod-security-policy.md](安全/概念-pod-security-policy.md)、[安全/字典-pod-security-*.md](安全/) | PSP/PSA/PSS 字典条目 |

### 📋 清单规范 · [清单规范/](清单规范/)

| 文件 | 说明 |
|------|------|
| [清单规范/03-pod-specification-complete.md](清单规范/03-pod-specification-complete.md) | Pod 规格完整参考 |
| [清单规范/12-advanced-pod-patterns.md](清单规范/12-advanced-pod-patterns.md)、[清单规范/35-advanced-pod-patterns.md](清单规范/35-advanced-pod-patterns.md) | 高级 Pod 模式 |
| [清单规范/23-pod-security-standards.md](清单规范/23-pod-security-standards.md) | 清单中的 PSS |
| [清单规范/28-poddisruptionbudget-reference.md](清单规范/28-poddisruptionbudget-reference.md) | PodDisruptionBudget 参考 |

### ⚙️ 配置与字典 · [配置与字典/](配置与字典/)

| 文件 | 说明 |
|------|------|
| [配置与字典/advanced-pod-configuration.md](配置与字典/advanced-pod-configuration.md) | 高级 Pod 配置 |
| [配置与字典/dns-for-services-and-pods.md](配置与字典/dns-for-services-and-pods.md) | Service/Pod DNS |
| [配置与字典/pod-hostname.md](配置与字典/pod-hostname.md) | Pod 主机名 |
| [配置与字典/pod-group-policies.md](配置与字典/pod-group-policies.md) | Pod 组策略 |

### 🎫 工单案例 · [工单案例/](工单案例/)

12 篇真实生产工单案例，聚焦 Pod Pending（资源/污点/亲和性）与 Ingress Controller Pod 异常（404/502）等高频场景。

### 🎓 培训 · [培训/](培训/)

7 篇培训材料，涵盖内训（inner）、公开课（public）、讲师版（lecturer）与学习版（learn）的 Pod 基础与进阶。

### ☁️ 云厂商 · [云厂商/](云厂商/)

| 文件 | 说明 |
|------|------|
| [云厂商/05-eks-iam-irsa-pod-identity.md](云厂商/05-eks-iam-irsa-pod-identity.md) | AWS EKS IAM/IRSA/Pod Identity |

### 🗂️ 生态索引 · [生态索引/](生态索引/)

| 文件 | 说明 |
|------|------|
| [生态索引/pod-index.md](生态索引/pod-index.md) | Pod 知识图谱领域索引 |

### 📦 上游源码文档 · [上游源码/](上游源码/)

来自上游项目仓库的一手文档。

| 目录 | 说明 |
|------|------|
| [上游源码/kruise-proposals/](上游源码/kruise-proposals/) | OpenKruise 提案（PUB、PersistentPodState、SidecarSet patch、PodProbeMarker 等，5 篇） |
| [上游源码/kubectl-docs/1.18/](上游源码/kubectl-docs/1.18/)、[1.20/](上游源码/kubectl-docs/1.20/) | kubectl 文档（Pod 模板定制、端口转发） |

---

## 快速诊断入口

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看异常 Pod 概览
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

# Step 2: 查看特定 Pod 详情
kubectl describe pod <pod-name> -n <namespace> | tail -30

# Step 3: 查看容器日志（含上次崩溃）
kubectl logs <pod-name> -n <namespace> --previous --tail=50

# Step 4: 检查退出码
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[*].lastState.terminated.exitCode}'
```

---

## Pod 状态速查表

| 状态 | 含义 | 常见原因 | 优先检查项 | 对应技能 |
|:---|:---|:---|:---|:---|
| **Pending** | 等待调度 | 资源不足/节点选择器/亲和性/污点 | `kubectl describe pod` Events | 02、诊断排障/05 |
| **ContainerCreating** | 创建容器中 | CNI/存储挂载/镜像拉取 | Events + kubelet 日志 | 03 |
| **CrashLoopBackOff** | 反复崩溃 | 应用错误/配置错误/依赖缺失 | `kubectl logs --previous` | 01 |
| **ImagePullBackOff** | 镜像拉取失败 | 镜像不存在/凭证错误/网络问题 | 镜像名/Secret/网络 | 03 |
| **OOMKilled** | 内存溢出 | 内存 limit 过小/内存泄漏 | `kubectl describe pod` Last State | 01、资源与自动扩缩 |
| **Init:Error** | 初始化失败 | InitContainer 错误 | InitContainer 日志 | 03 |
| **Terminating** | 删除中 | Finalizer/PV 卸载/preStop 阻塞 | Finalizers/Events | 04 |
| **Evicted** | 被驱逐 | 资源压力/节点维护 | 节点状态/资源 | 04、诊断排障/案例 |

---

## FTA 故障树路径映射

| 顶层事件 | 中间事件 | 底事件 | 对应技能 |
|---------|---------|--------|---------|
| TE-2 应用服务不可用 | IE-2.1 Pod 运行异常 | BE-2.1 CrashLoopBackOff | 01 |
| TE-2 应用服务不可用 | IE-2.1 Pod 运行异常 | BE-2.3 OOMKilled | 01 |
| TE-3 Pod 启动失败 | IE-3.1 调度失败 | BE-3.1~3.4 | 02 |
| TE-3 Pod 启动失败 | IE-3.2 镜像拉取失败 | BE-3.5~3.7 | 03 |
| TE-3 Pod 启动失败 | IE-3.3 容器创建失败 | BE-3.8~3.10 | 03 |

完整故障树见 [pod-fta.md](pod-fta.md) 与 [诊断排障/pod-fta-故障树list.md](诊断排障/pod-fta-故障树list.md)。

---

## Kubernetes 版本兼容性矩阵

本技能集基于 `code/` 目录中 Kubernetes 1.18/1.20/1.28/1.30/1.32/1.34/1.36 源码（`pkg/apis/core/types.go`）实证比对。各诊断技能适用版本范围如下：

| 技能 | 适用版本范围 | 版本敏感点 |
|------|------------|---------|
| 01 CrashLoop/OOMKilled | 全版本通用（1.18–1.36） | `kubectl debug` 临时容器 1.25 GA；Sidecar 容器 1.28+ 逐步可见；`.status.resize` 在 1.34 废弃 |
| 02 Pending/调度失败 | 全版本通用；`SchedulingGated` 需 1.28+ | `SchedulingGates` 1.28 beta→1.34 GA；DRA/Gang 调度需 1.36+ |
| 03 镜像拉取/容器创建 | 全版本通用 | 1.24+ 节点运行时为 containerd/CRI-O，需用 `crictl` 而非 `docker` |
| 04 SOP/Runbook | 全版本通用 | `DisruptionTarget` 1.28+；StopSignal 1.34+ |

> 完整的特性×版本矩阵、PodSpec/PodStatus 字段演进及存疑标注请参阅 [reference/pod-version-differences.md](reference/pod-version-differences.md)。
>
> **通用提示**：排障前先执行 `kubectl version -o json` 确认集群版本。本技能集核心命令（`kubectl get/describe/logs/get events`）在 1.18–1.36 全版本输出主体结构一致，版本差异主要体现在**新增字段的有无**。

---

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[技能/排障实战/workloads/ts-workloads.md|工作负载故障排查]]
- [[故障诊断/README.md|Domain-12 故障排查]]
- [[生态参考/领域索引/pod-index.md|Pod 知识图谱索引]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[generated/pod-index|Pod 知识图谱索引]]
