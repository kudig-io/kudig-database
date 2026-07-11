---
title: containerd-shim-runc-v2 架构与调试
description: containerd-shim-runc-v2 进程模型、与 containerd 的关系、故障定位与调试技巧
summary: containerd-shim-runc-v2 进程模型、与 containerd 的关系、故障定位与调试技巧
category: container-runtime
tags:
- containerd
- cri
- runtime
- shim
- runc
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

# containerd-shim-r2 架构与调试

## 概述

`containerd-shim-runc-v2`（下称 shim v2）是 containerd 与容器之间的"中间人"。每个 Pod（一组共享 namespace 的容器）由一个 shim 进程托管。shim 让 containerd 守护进程可以升级、重启，而不影响已运行的容器——这是 containerd 相对 dockerd 的关键架构优势。

## 进程模型

```
containerd (1 per node)
 ├─ containerd-shim-runc-v2 (1 per Pod/Sandbox)
 │   ├─ runc (create→start 后即退出)
 │   └─ 容器进程（业务 PID）
 └─ ttrpc 管理所有 shim
```

- shim 通过 **ttrpc**（紧凑型 gRPC）与 containerd 通信
- shim 持有容器的 stdout/stderr pipe、exit FIFO、reaper（收尸）职责
- shim 父进程退出后由操作系统重新挂在 `init`（或 systemd）下，containerd 重启后通过 `address.sock` 重连

## shim v1 vs v2

| 维度 | shim v1 (runc) | shim v2 |
|---|---|---|
| 容器/shim 比 | 1 容器 : 1 shim | 1 Pod : 1 shim（多容器共享） |
| containerd 2.0 | 不支持 | 唯一支持 |
| 内存开销 | 高（每容器一进程） | 低（Pod 级聚合） |
| 进程数 | N | N/P（P=每 Pod 容器数） |

> v2 在大 Pod（多 sidecar）节点显著降低 shim 进程数与内存占用。

## 与 containerd 的解耦

容器运行时栈解耦三层：

```
containerd   ── 可重启/升级（不杀容器）
   │ ttrpc
shim v2      ── 容器生命周期托管、收尸、日志管道
   │ exec
runc         ── 创建 OCI 容器后即退出（后续由 shim reap）
   │
容器进程
```

验证解耦：重启 containerd 后，业务 Pod 不重启，仅日志短暂中断。

> ⚠️ **🟠 高危操作** — 节点级影响

``` bash
# 🔴 高风险：重启 containerd 会让节点短暂 NotReady
sudo systemctl restart containerd
# 验证容器仍在运行（PID 不变）
crictl ps
```

## 调试 shim

``` bash
# 🟢 只读：定位 Pod → shim → 容器
crictl pods                                # 取 sandbox-id
crictl inspectp <sandbox-id> | grep -i pid # 沙箱 PID（即 shim 父）
ls -l /proc/<shim-pid>/exe                 # 确认 shim 二进制
# shim 的 socket 地址
crictl inspect <container-id> | grep -i address
```

``` bash
# 🟢 只读：查看 shim 进程与子进程树
pstree -p <shim-pid>
# 容器真实 PID（在 host namespace）
crictl inspect <container-id> | jq '.info.pid'
```

## 常见故障

| 现象 | 根因 | 处理 |
|---|---|---|
| `failed to create shim` | shim 二进制缺失/损坏 | 重装 containerd.io，校验 `/usr/local/bin/containerd-shim-runc-v2` |
| shim 进程僵尸 | runc 卡住、FIFO 未关闭 | 查 `journalctl -u containerd`，必要时 `kill` shim（容器会退出） |
| 容器退出但 shim 残留 | shim 未收到 SIGCHLD reaper | 升级 containerd，已知 bug 多在旧版本 |
| `ttrpc: connection closed` | containerd 崩溃，shim 失联 | 重启 containerd，shim 会重连 |

## 日志与 metrics

shim 日志写入 containerd journal（`journalctl -u containerd`）。shim 自身暴露 metrics 给 containerd 聚合：

```
container_runtime_container_shim_cpu_usage_seconds_total
container_runtime_container_shim_memory_usage_bytes
```

监控 shim 内存异常增长，可提前发现泄漏。

## 生产检查清单

- [ ] `containerd-shim-runc-v2` 二进制存在且可执行
- [ ] containerd 升级演练验证"重启不杀容器"
- [ ] 监控 shim 进程数与内存，设置异常告警
- [ ] 已知 shim bug 版本已升级规避

## 相关文档

- [[容器运行时/containerd-CRI-O/09-container-runtime-lifecycle.md|容器运行时生命周期]]
- [[容器运行时/containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]]
- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]

<!-- risk-assessed -->
