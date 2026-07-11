---
title: 容器运行时生命周期
description: 从 Pod Sandbox 创建到容器启动、停止、删除的完整 CRI 生命周期，含每阶段排障要点
summary: 从 Pod Sandbox 创建到容器启动、停止、删除的完整 CRI 生命周期，含每阶段排障要点
category: container-runtime
tags:
- containerd
- cri
- runtime
- lifecycle
- kubernetes
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace；RBAC 权限；非生产环境验证。风险标注：🔴 高风险 / 🟡 中风险 / 🟢 只读。

# 容器运行时生命周期

## 概述

一个 Pod 在节点上的生命周期由一系列 CRI 调用驱动：Sandbox 创建 → 容器创建 → 启动 → 停止 → 删除。每个阶段都可能出现卡住或失败。本文按阶段拆解，给出每段对应的 `crictl` 验证命令与常见根因。

## 生命周期总览

```
kubelet SyncPod
 1. RunPodSandbox      ── pause 容器 + Pod CNI 网络
 2. PullImage(s)       ── 按需拉取业务镜像
 3. CreateContainer    ── rootfs / OCI bundle
 4. PreStartHook       ── 设备、cgroup（可选）
 5. StartContainer     ── runc create + start
 ── 运行中（Running） ──
 6. StopContainer      ── SIGTERM → grace → SIGKILL
 7. RemoveContainer
 8. StopPodSandbox
 9. RemovePodSandbox
```

## 阶段 1：Pod Sandbox

`RunPodSandbox` 创建 pause（infra）容器并隔离出 Pod 级 network/IPC/UTS namespace，随后调用 CNI 插件接入网络。

``` bash
# 🟢 低风险：只读/信息收集
crictl pods --name <pod-name>          # 查看沙箱状态
crictl inspectp <sandbox-id>           # 查看沙箱网络/namespace
```

常见失败：

| 现象 | 根因 | 处理 |
|---|---|---|
| `SandboxCreate` 卡住 | CNI 插件缺失或 /opt/cni/bin 不全 | 重装/补齐 CNI 二进制 |
| `failed to setup network` | IPAM 地址耗尽 | 扩容 CIDR 或清理泄露 IP |
| `pull pause image timeout` | sandbox_image 不可达 | 配置内网 pause 镜像 |

## 阶段 2：镜像拉取

``` bash
# 🟢 低风险：只读
crictl images | grep <image>
crictl pull <image>                    # 手动验证仓库连通
```

Pod 卡在 `ContainerCreating` 且事件含 `pulling image`，多为仓库认证/网络问题，参见镜像拉取排查。

## 阶段 3：容器创建

`CreateContainer` 在沙箱 namespace 内准备 rootfs（snapshotter mount）、写入 OCI `config.json`，但尚未 `runc create`。

``` bash
# 🟢 低风险：只读
crictl ps -a --state created           # 已创建未启动
crictl inspect <container-id>
```

常见失败：

- `failed to create shim`：runc/shim 二进制损坏或 `/run/containerd` 权限错乱
- `mount /conf failed`：ConfigMap/Secret 卷挂载失败（API server 不可达）
- `OOMKilled` 出现在启动后：limit 过低，`crictl inspect` 看 `exitCode=137`

## 阶段 4：启动与运行

`StartContainer` 调用 `runc create` → `runc start`，容器进入 Running。此时 shim（`containerd-shim-runc-v2`）接管容器生命周期，containerd 重启不影响运行容器。

``` bash
# 🟢 低风险：只读
crictl ps --state running
crictl stats                            # CPU/内存实时
crictl logs <container-id>
```

## 阶段 5：停止与删除

`StopContainer` 先发 `SIGTERM`，等待 `terminationGracePeriodSeconds`（默认 30s）后发 `SIGKILL`。

> ⚠️ **🟠 高危操作** — 影响业务，需变更窗口

``` bash
# 🟡 中风险：停止业务容器
crictl stop <container-id>
crictl rm <container-id>
crictl stopp <sandbox-id> && crictl rmp <sandbox-id>
```

## GC 与回收

kubelet 周期性调用 `RemovePodSandbox` / `RemoveImage`，阈值由 kubelet `--image-gc-high-threshold`（默认 85%）与 `--image-gc-low-threshold`（默认 80%）控制。容器退出码、重启计数通过 `ContainerStatus` 上报，驱动 CrashLoopBackOff 退避。

## 排障速查表

| Pod 阶段 | 卡住点 | 第一排查命令 |
|---|---|---|
| Pending | 调度失败 | `kubectl describe pod`（Events） |
| ContainerCreating | Sandbox/镜像 | `crictl pods` + `crictl images` |
| ContainerCreating | 卷挂载 | `crictl inspect <c>` 看 mounts |
| Running→CrashLoop | 启动失败 | `crictl logs` + `crictl inspect` exitCode |
| Terminating | 停止超时 | `crictl ps -a --state exited` + finalizer |

## 生产检查清单

- [ ] 节点 CNI 二进制齐全且版本匹配
- [ ] pause 镜像走内网，避免拉取阻塞沙箱创建
- [ ] kubelet GC 阈值与磁盘容量匹配
- [ ] 关键 Pod 配置合理的 `terminationGracePeriodSeconds`

## 相关文档

- [[容器运行时/containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]]
- [[容器运行时/containerd-CRI-O/12-container-shim-v2.md|containerd-shim-runc-v2]]
- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]

<!-- risk-assessed -->
