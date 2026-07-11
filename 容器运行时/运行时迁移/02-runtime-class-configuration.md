---
title: RuntimeClass 配置指南
description: 使用 RuntimeClass 将 gVisor / Kata / runsc 等 handler 按工作负载分配，含节点 RuntimeHandler 与 Pod 绑定
summary: 使用 RuntimeClass 将 gVisor / Kata / runsc 等 handler 按工作负载分配，含节点 RuntimeHandler 与 Pod 绑定
category: container-runtime
tags:
- containerd
- cri
- runtime
- runtimeclass
- gvisor
- kata
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 平台工程师
---

> **生产环境安全提示**
>
> 风险标注：🔴 高风险 / 🟡 中风险 / 🟢 只读。

# RuntimeClass 配置指南

## 概述

集群中常混用多种容器运行时：默认 runc（高性能、共享内核）、gVisor（用户态内核、强隔离）、Kata（轻量 VM、最强隔离）。**RuntimeClass** 让你在同一集群里按工作负载指定运行时：不受信任的代码用 gVisor，合规要求高的用 Kata，普通业务用 runc——无需为每个 Pod 单独配节点。

## 工作机制

```
Pod.spec.runtimeClassName → RuntimeClass(node.k8s.io) → handler
   ↓ 调度
节点 containerd [plugins."...".containerd.runtimes.<handler>]
   ↓
对应 OCI runtime（runc / runsc / kata）
```

- RuntimeClass 是集群级 API 对象，`handler` 字段对应节点 containerd 配置里的 `runtimes.<name>`。
- 调度可通过 `RuntimeClass.scheduling` 选择装了对应 runtime 的节点（`nodeSelector`/`tolerations`）。

## 节点侧：注册 RuntimeHandler

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
  runtime_type = "io.containerd.runsc.v1"

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
  runtime_type = "io.containerd.kata.v2"
```

``` bash
# 🟢 只读：确认节点已识别 handler
crictl info | jq '.config.containerd.runtimes | keys'
```

## 创建 RuntimeClass

``` yaml
# gvisor.yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
scheduling:
  nodeSelector:
    sandbox-runtime: gvisor   # 仅调度到装了 runsc 的节点
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
scheduling:
  nodeSelector:
    sandbox-runtime: kata
```

> ⚠️ **🟡 中危变更**

``` bash
# 🟡 中风险：创建集群级对象
kubectl apply -f gvisor.yaml
kubectl get runtimeclass
```

## Pod 绑定 RuntimeClass

``` yaml
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-job
spec:
  runtimeClassName: gvisor        # ← 一行即可
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/demo/runner:v1
```

``` bash
# 🟢 只读：确认实际使用的 runtime
kubectl get pod untrusted-job -o jsonpath='{.status.runtimeHandler}{"\n"}'
crictl inspectp <sandbox-id> | jq '.info.runtimeType'
```

## 节点标签与调度隔离

``` bash
# 🟡 中风险：给节点打标签（控制调度）
kubectl label node node-arm-sandbox sandbox-runtime=gvisor
# 容忍：让 RuntimeClass Pod 只跑在专用节点
```

若节点未装 handler 但 Pod 指定了 RuntimeClass，会报 `RuntimeClass "gvisor" not found` 或 `handler not registered`。

## handler 选型

| 场景 | 推荐 handler | 隔离强度 | 性能损耗 |
|---|---|---|---|
| 普通业务 | runc | 内核共享 | 无 |
| 不受信任代码 / SaaS 多租 | runsc (gVisor) | 用户态内核 | 中（10-30%） |
| 合规 / 金融强隔离 | kata | 轻 VM | 高（启动慢） |
| Serverless 敏感 | firecracker-containerd | microVM | 中 |

## 常见故障

| 现象 | 根因 | 处理 |
|---|---|---|
| `RuntimeClass not found` | 集群未创建该 RC | `kubectl get runtimeclass` |
| `handler not registered` | 节点 containerd 未配 runtimes.<handler> | 补 config.toml 并重启 containerd |
| Pod Pending | 调度 `nodeSelector` 无匹配节点 | 给节点打标签或扩容专用池 |
| `runsc: operation not permitted` | runsc 需特定内核能力 | 升级 runsc，检查 kernel ≥ 5.4 |

## 生产检查清单

- [ ] 节点 containerd 注册了所有业务所需 handler
- [ ] RuntimeClass 配 `scheduling.nodeSelector` 隔离专用节点池
- [ ] 关键 Pod 已设 `runtimeClassName`，并验证 `.status.runtimeHandler`
- [ ] runsc / kata 二进制版本与内核匹配，已通过冒烟测试

## 相关文档

- [[容器运行时/05-gvisor-sandbox-production.md|gVisor 生产指南]]
- [[容器运行时/06-firecracker-microvm-guide.md|Firecracker microVM]]
- [[容器运行时/containerd-CRI-O/04-kata-containers-secure-container.md|Kata Containers]]
- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]

<!-- risk-assessed -->
