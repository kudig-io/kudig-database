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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| shim 进程泄漏 | 容器删除后 shim 未退出 | `ps aux | grep shim | wc -l` | 升级 containerd，检查 shim bug |
| containerd 重启后容器丢失 | shim 版本不兼容 | `containerd-shim-runc-v2 --version` | 确认 shim 二进制与 containerd 版本匹配 |
| 容器启动超时 | shim 初始化失败 | `journalctl -u containerd | grep shim` | 检查 runc 二进制和 cgroup 配置 |
| shim 内存持续增长 | 内存泄漏 | `cat /proc/<shim-pid>/status | grep VmRSS` | 升级到已修复版本 |
| OOM Kill 影响 shim | cgroup 配置不当 | `dmesg | grep -i oom` | 将 shim 放入独立 cgroup |
| exec 进入容器失败 | shim ttrpc 连接断开 | `crictl exec <id> /bin/sh` | 检查 shim 进程状态，必要时重建容器 |
| 大量僵尸 shim 进程 | 节点资源不足 | `ps -eo pid,ppid,stat | grep Z` | 清理僵尸进程，检查节点负载 |
| shim 日志无输出 | 日志级别配置错误 | 检查 /etc/containerd/config.toml | 设置 `debug.level = "debug"` |

## Shim v2 架构详解

```text
Shim v2 架构层次：

containerd daemon
  └── ttrpc (Unix Socket)
       └── containerd-shim-runc-v2 (每容器一个进程)
            ├── 管理容器进程生命周期
            ├── 处理 exec/attach 请求
            ├── 收集容器 metrics
            ├── 管理容器 stdio
            └── 调用 runc 执行实际操作
                 └── runc (OCI runtime)
                      └── 容器进程
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 版本管理 | shim 与 containerd 同版本升级 | 避免 ABI 不兼容 |
| 监控 | 监控 shim 进程数和内存 | 异常增长及时告警 |
| 升级演练 | 验证“containerd 重启不杀容器” | shim 的核心价值 |
| 资源限制 | 为 shim 设置独立 cgroup | 避免被容器 OOM 连带 |
| 日志 | 生产环境 info 级别 | debug 仅用于排障 |
| 清理 | 定期检查僵尸 shim | 防止资源泄漏 |
| 二进制 | 确认 shim 二进制权限 755 | 避免权限问题导致启动失败 |
| 回滚 | 保留上一版本 shim 二进制 | 升级失败时快速回滚 |

## Shim 版本兼容性

| containerd 版本 | shim 版本 | runc 最低版本 | 说明 |
|----------------|-----------|-------------|------|
| 1.6.x | shim-runc-v2 1.6.x | runc 1.1.x | 稳定版 |
| 1.7.x | shim-runc-v2 1.7.x | runc 1.1.x | 性能优化 |
| 2.0.x | shim-runc-v2 2.0.x | runc 1.2.x | 新架构 |

## 相关工具

| 工具 | 用途 | 使用方式 |
|------|------|----------|
| containerd-shim-runc-v2 | 默认 shim 实现 | 随 containerd 安装 |
| containerd-shim-kata-v2 | Kata 容器 shim | 随 kata-containers 安装 |
| containerd-shim-spin-v2 | Spin/Wasm shim | 随 spin 安装 |
| ttrpc | shim 通信协议 | 内置于 containerd |
| runc | OCI 运行时 | 随 containerd 安装 |
| crun | 轻量 OCI 运行时 | `dnf install crun` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| shim v1 和 v2 的区别？ | v2 每容器一个进程（v1 每容器一个），v2 支持 ttrpc 更高效 |
| 为什么 containerd 重启不杀容器？ | shim 独立进程持有容器，与 containerd 解耦 |
| 如何查看 shim 进程？ | `ps aux | grep containerd-shim` |
| shim 占用多少内存？ | 通常 5-15MB/容器，异常增长表示泄漏 |
| 如何自定义 shim？ | 实现 ttrpc TaskService 接口，注册到 containerd |
| shim 日志在哪里？ | 默认 journalctl，可配置到文件 |
| 如何强制清理 shim？ | `kill -9 <shim-pid>`，containerd 会自动清理关联资源 |
| 多运行时如何共存？ | 配置多个 runtime_type，每个对应不同 shim |

## 相关文档

- [[容器运行时/containerd-CRI-O/09-container-runtime-lifecycle.md|容器运行时生命周期]]
- [[容器运行时/containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]]
- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]

<!-- risk-assessed -->
