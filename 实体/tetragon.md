---
title: Tetragon
description: Tetragon — Kubernetes 生产运维知识库
summary: Tetragon — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- runtime
- ebpf
- tetragon
- monitoring
- cilium
- falco
- networkpolicy
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tetragon 是什么
- 如何 Tetragon
trigger_keywords:
- Tetragon
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tetragon

> **CNCF 状态**: Sandbox | **类别**: Security/Runtime | **主要语言**: Go, C

## 概述

Tetragon 是由 Isovalent（Cilium 团队）开发的 eBPF 安全可观测与执行平台，2023 年加入 CNCF 沙箱。它利用 eBPF 技术在 Linux 内核中实现实时的安全策略执行（Enforcement）和深度可观测性（Observability）。与基于用户态日志分析的 Falco 不同，Tetragon 直接在内核态拦截和执行安全策略，能够以零延迟阻止恶意行为（如阻止特定进程执行、阻止异常网络连接）。Tetragon 通过 TracingPolicy CRD 定义安全策略，可以监控进程执行、文件访问、网络连接和 Linux capability 使用等内核事件，支持 Log（记录）、Enforce（阻断）和 Signal（通知）三种响应模式。

## 核心能力

- **实时安全执行**: 在内核态直接阻断恶意行为（如 kill 进程、drop 连接），零用户态延迟
- **进程追踪**: 监控所有容器内的进程创建、执行和终止
- **文件访问监控**: 追踪对敏感路径（/etc/passwd、/etc/shadow）的读写操作
- **网络连接追踪**: 检测异常出站连接（如反弹 shell、数据外泄）
- **Capability 审计**: 追踪通过 Linux capabilities 的权限提升行为
- **TracingPolicy CRD**: Kubernetes 原生的安全策略定义

## 架构

Tetragon 基于 eBPF 的内核态安全引擎：

- **Tetragon Agent**: 每个节点上运行的 DaemonSet，管理 eBPF 程序生命周期
- **eBPF Programs**: 挂载在内核 hook 点（tracepoint、kprobe、LSM）的 eBPF 字节码
- **TracingPolicy CRD**: 声明式安全策略，定义监控事件和响应动作
- **Policy Engine**: 解析 TracingPolicy，生成并加载对应的 eBPF 程序
- **Event Queue**: 内核事件缓冲区，将安全事件从内核传递到用户态
- **Export**: 安全事件通过 gRPC/JSON 导出到 SIEM/Prometheus

事件流：`内核事件 → eBPF (filter/match) → 用户态 (log/enforce) → SIEM/告警`

## K8s 集成

Tetragon 以 DaemonSet 方式部署在 Kubernetes 集群的所有节点上。每个节点上的 Tetragon Agent 通过 eBPF 程序监控内核事件。安全策略通过 TracingPolicy CRD 定义，创建后自动分发到所有节点的 Agent。Agent 将 eBPF 程序加载到内核，在事件匹配时执行对应动作（Log/Enforce/Signal）。安全事件通过 Tetragon CLI、JSON 导出或 Prometheus 指标查看。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 NetworkPolicy（L3/L4）互补，Tetragon 提供 L7 和进程级别的安全控制。

## 生产场景

1. **运行时安全防护**: 阻止容器内反弹 shell、挖矿程序执行等攻击行为
2. **合规审计**: 记录所有对敏感文件和密钥的访问，满足 PCI-DSS、SOC 2 要求
3. **零日漏洞防护**: 在内核态阻止利用漏洞的异常行为，无需等待补丁
4. **入侵检测（IDS/IPS）**: 实时检测和阻断容器逃逸、权限提升攻击

## 安装

```bash
# Helm 安装 Tetragon
helm repo add cilium https://helm.cilium.io/
helm install tetragon cilium/tetragon -n kube-system

# 安装 Tetragon CLI
go install github.com/cilium/tetragon/cmd/tetra@latest

# 查看实时安全事件
kubectl exec -n kube-system ds/tetragon -- tetra getevents

# 部署安全策略：阻止容器内执行 /bin/bash
kubectl apply -f - <<EOF
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: block-shell-exec
spec:
  kprobes:
  - call: "sys_execve"
    syscall: true
    args:
    - index: 0
      type: "string"
    matchArgNames:
    - operator: "Equal"
      values:
      - "/bin/bash"
      - "/bin/sh"
    selectors:
    - matchActions:
      - action: Sigkill
EOF
```

## 对比

| 特性 | Tetragon | Falco | Tracee | KubeArmor |
|------|----------|-------|--------|-----------|
| 内核态执行 | ✅ Enforce | ❌ Log only | ✅ | ✅ |
| eBPF | ✅ | ✅ | ✅ | ⚠️ LSM |
| 响应延迟 | 零（内核态） | 高（用户态） | 低 | 低 |
| CNCF 状态 | Sandbox | Graduated | 非 CNCF | Sandbox |

## 架构定位

在 CNCF 生态中，Tetragon 属于 **Security** 类别，为云原生应用提供基于 eBPF 的实时安全执行能力。

## Related

- [[confidential-containers]] — Confidential Containers (CoCo)
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[bootc]] — bootc
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[cilium]] — Cilium
- [[概念/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[falco|Falco]]
- [[cilium|Cilium]]

- 06-tetragon-runtime-security

<!-- risk-assessed -->
