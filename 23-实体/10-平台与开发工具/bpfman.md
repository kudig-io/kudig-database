---
title: bpfman (entities)
description: '## 概述'
summary: 'bpfman 是一个 eBPF 程序管理器，提供系统守护进程和 Kubernetes Operator，用于集中加载、管理和监控 eBPF 程序。'
category: entities
tags:
- k8s
- cncf
- networking
- bpfman
- cilium
- argocd
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bpfman 是什么
- 如何 bpfman
trigger_keywords:
- bpfman
prerequisites:
- kubectl-basics
- gitops-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# bpfman

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Rust

## 概述

bpfman 是一个 eBPF 程序管理器，由 Red Hat 推动开发，2023 年加入 CNCF 沙箱。它提供系统守护进程和 Kubernetes Operator，用于集中加载、管理和监控 eBPF 程序。bpfman 解决了多个应用（如 Cilium、Falco、Tetragon）同时使用 eBPF 时的管理混乱问题——这些程序各自加载 eBPF bytecode，可能导致 hook 冲突和资源竞争。bpfman 提供统一的 eBPF 程序生命周期管理、多程序共享挂载点、权限控制和可观测性。它支持将 eBPF bytecode 打包为 OCI 镜像通过 Registry 分发，实现了 eBPF 程序的云原生部署和版本管理。

## 核心能力

- **统一 eBPF 管理**: 集中加载、卸载和监控所有 eBPF 程序，避免冲突
- **OCI 镜像分发**: 将 eBPF bytecode 打包为 OCI 镜像，通过 Registry 管理版本
- **Kubernetes Operator**: 通过 CRD 声明式管理 eBPF 程序部署
- **程序类型支持**: XDP、TC、Tracepoint、Kprobe、Uprobe、Perf Event 等
- **优先级控制**: 为多个 eBPF 程序设置执行优先级，确保关键程序优先
- **可观测性**: 暴露 Prometheus 指标，跟踪 eBPF 程序加载状态和错误

## 架构

bpfman 采用守护进程 + Operator 双模式架构：

- **bpfman (守护进程)**: 运行在每个节点上的系统守护进程，负责实际的 eBPF 程序加载和管理
- **bpfman-agent**: 运行在节点上的 gRPC 接口，接收来自 Kubernetes 的指令
- **bpfman-operator**: Kubernetes Operator，通过 CRD 管理集群范围的 eBPF 程序部署
- **BpfProgram CRD**: 声明式定义 eBPF 程序（bytecode 来源、挂载点、参数）
- **OCI Image**: eBPF bytecode 以 OCI 镜像格式存储在 Registry 中
- **Map 管理**: 共享 eBPF Maps，支持多程序间通信

管理流程：`BpfProgram CRD → Operator → bpfman-agent → bpfman → 加载 eBPF bytecode 到内核`

## K8s 集成

bpfman 通过 Kubernetes Operator 深度集成集群管理。bpfman-operator 监听 `BpfProgram` CRD，将期望的 eBPF 程序分发到目标节点（通过 nodeSelector/affinity 控制）。每个节点运行 bpfman 守护进程和 bpfman-agent，agent 通过 gRPC 与 Operator 通信。eBPF bytecode 以 OCI 镜像形式存储在 Registry（如 Harbor）中，节点拉取后通过 bpfman 加载到内核。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 DaemonSet 模式类似，确保所有目标节点都运行指定的 eBPF 程序。

## 生产场景

1. **安全监控部署**: 统一部署 Tetragon/Falco 类安全 eBPF 程序，避免与 CNI 的 eBPF 程序冲突
2. **网络观测**: 部署 XDP/TC eBPF 程序进行网络流量监控和过滤
3. **性能分析**: 动态部署 perf event eBPF 程序进行性能 profiling，无需重启节点
4. **多团队 eBPF 共存**: 不同团队（网络、安全、可观测性）的 eBPF 程序通过 bpfman 统一管理

## 安装与配置

### Operator 部署

```bash
# 安装 bpfman Operator
kubectl apply -f https://github.com/bpfman/bpfman/releases/latest/download/bpfman-operator.yaml

# 验证部署状态
kubectl get pods -n bpfman
kubectl get crd | grep bpfman
```

### 部署 eBPF 程序（XDP 示例）

```yaml
apiVersion: bpfman.io/v1alpha1
kind: XdpProgram
metadata:
  name: xdp-pass-all
spec:
  bpffunctionname: pass
  interfaceselector:
    primarynodeinterface: true
  priority: 0
  bytecode:
    image:
      url: quay.io/bpfman-bytecode/xdp_pass:latest
---
# TC 程序示例
apiVersion: bpfman.io/v1alpha1
kind: TcProgram
metadata:
  name: tc-pass-eth0
spec:
  bpffunctionname: pass
  interfaceselector:
    interfaces:
      - eth0
  direction: ingress
  priority: 0
  bytecode:
    image:
      url: quay.io/bpfman-bytecode/tc_pass:latest
```

### 查看已加载的 eBPF 程序

```bash
# 查看集群中所有 BpfProgram
kubectl get bpfprograms -A
kubectl get xdpprograms -A
kubectl get tcprograms -A

# 查看特定节点上的程序状态
kubectl get bpfprograms -l bpfman.io.xdpprogramcontroller=xdp-pass-all
```

## 运维操作

```bash
# 🟢 查看节点上已加载的 eBPF 程序
kubectl exec -n bpfman daemonset/bpfman-daemon -- bpctl list

# 🟢 查看 eBPF Map 内容
kubectl exec -n bpfman daemonset/bpfman-daemon -- bpctl map get <map-id>

# 🟡 部署新的 eBPF 程序
kubectl apply -f xdp-program.yaml

# 🟡 更新 eBPF 程序版本（修改 bytecode image URL）
kubectl patch xdpprogram xdp-pass-all --type merge -p '{"spec":{"bytecode":{"image":{"url":"quay.io/bpfman-bytecode/xdp_pass:v2"}}}}'

# 🔴 卸载 eBPF 程序（删除 CRD 实例）
kubectl delete xdpprogram xdp-pass-all

# 🔴 重启 bpfman 守护进程（会重新加载所有程序）
kubectl rollout restart daemonset/bpfman-daemon -n bpfman
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| BpfProgram 状态为 NotLoaded | bytecode 镜像拉取失败 | `kubectl describe bpfprogram <name>` | 检查镜像地址和拉取权限 |
| 节点上程序未生效 | nodeSelector 不匹配 | `kubectl get bpfprogram -l kubernetes.io/hostname=<node>` | 检查节点标签和选择器 |
| eBPF 程序加载失败 | 内核版本不兼容 | `journalctl -u bpfman -f` | 确认内核 >= 5.15，检查 BTF 支持 |
| 与 Cilium eBPF 冲突 | XDP 优先级冲突 | `bpctl list` 查看优先级 | 调整 priority 值避免冲突 |
| Operator CrashLoop | CRD 版本不匹配 | `kubectl logs -n bpfman -l app=bpfman-operator` | 重新应用 CRD 定义 |

**排查流程：**
```
eBPF 程序未生效
├── 检查 BpfProgram CR 状态 → kubectl get bpfprograms -A
├── 检查 bpfman 守护进程日志 → journalctl -u bpfman
├── 检查 bytecode 镜像拉取 → crictl pull <image>
├── 检查内核兼容性 → uname -r && ls /sys/kernel/btf/vmlinux
└── 检查优先级冲突 → bpctl list | grep xdp
```

## 生产案例

### 案例一：多团队 eBPF 程序共存

- **场景**: 网络团队（Cilium XDP）、安全团队（Tetragon tracepoint）、可观测团队（perf event）同时使用 eBPF，经常发生 hook 冲突
- **排查**: 各团队独立加载 eBPF 程序，无统一管理，导致 XDP 程序被覆盖
- **方案**: 部署 bpfman 统一管理，为各团队程序设置优先级（Cilium priority=50, Tetragon priority=100），通过 OCI 镜像版本管理 bytecode
- **效果**: 消除 eBPF 程序冲突，程序加载时间从 30s 降至 5s，支持热更新无需重启节点

### 案例二：动态性能分析

- **场景**: 生产环境某服务延迟突增，需要临时部署 perf event eBPF 程序进行 profiling
- **排查**: 传统方式需要登录节点手动加载，多节点操作耗时且有风险
- **方案**: 通过 bpfman CRD 声明式部署 perf profiling 程序到目标节点，分析完成后删除 CRD 即可卸载
- **效果**: 从发现到部署仅 2 分钟，无需 SSH 登录节点，符合生产环境变更管控要求

## 对比

| 特性 | bpfman | Cilium (内置) | Inspektor Gadget | bpfd | 适用场景 |
|------|--------|---------------|-----------------|------|----------|
| 统一管理 | ✅ 多源 | ❌ 仅自身 | ⚠️ 工具集 | ✅ | bpfman 多团队 |
| OCI 分发 | ✅ | ❌ | ❌ | ✅ | 版本管理 |
| K8s Operator | ✅ | ❌ | ✅ | ✅ | 声明式管理 |
| 语言 | Rust | Go | Go | Rust | - |
| 优先级控制 | ✅ | ❌ | ❌ | ✅ | 多程序共存 |
| 成熟度 | Sandbox | Graduated | Incubating | 已合并入 bpfman | - |

## 架构定位

在 CNCF 生态中，bpfman 属于 **Networking** 类别，为云原生应用提供统一的 eBPF 程序生命周期管理能力。

## 参考链接

- [[cilium]]
- [[23-实体/argocd.md|[[argocd|argocd]]]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]

## Related

- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[23-实体/04-网络/09-terway-performance-tuning]] — Terway 性能调优
- [[volcano]] — Volcano
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bpfman
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
