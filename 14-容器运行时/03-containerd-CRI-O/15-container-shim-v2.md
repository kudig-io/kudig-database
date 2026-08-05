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

## Shim 配置示例

```toml
# /etc/containerd/config.toml - 多运行时 shim 配置
[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

  # runc shim
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true
      BinaryName = "/usr/bin/runc"

  # kata shim
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata.options]
      ConfigPath = "/opt/kata/share/defaults/kata-containers/configuration.toml"

  # gvisor shim
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
    runtime_type = "io.containerd.runsc.v1"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options]
      TypeUrl = "io.containerd.runsc.v1.options"
```

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| shim 启动慢 | 预热二进制 | 节点初始化时预加载 |
| 内存占用高 | 检查泄漏 | 监控 VmRSS，升级版本 |
| ttrpc 超时 | 调整超时 | 修改 containerd 配置 |
| 大量 shim 进程 | 检查泄漏 | 定期清理僵尸进程 |
| exec 延迟高 | 检查 ttrpc | 确认 socket 连通性 |
| 日志过大 | 调整级别 | 生产用 info，排障用 debug |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| shim_count | shim 进程总数 | > 容器数 * 1.2 |
| shim_memory_bytes | shim 内存使用 | > 50MB/进程 |
| shim_cpu_seconds | shim CPU 使用 | 持续 > 50% |
| shim_start_duration | shim 启动耗时 | P99 > 2s |
| orphan_shim_count | 孤儿 shim 数 | > 0 |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 二进制权限 | 755，仅 root 可写 | 避免篡改 |
| cgroup 隔离 | shim 独立 cgroup | 避免被容器 OOM 连带 |
| 日志审计 | 记录关键操作 | 便于安全审计 |
| 版本管理 | 及时更新 | 修复已知漏洞 |
| 最小权限 | 移除不必要能力 | 减小攻击面 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| shim v1 | shim v2 | 升级 containerd→自动使用 v2 |
| runc shim | kata shim | 安装 kata→配置 runtime_type |
| runc shim | gvisor shim | 安装 runsc→配置 runtime_type |
| 单 shim | 多 shim | 配置多个 runtime 块 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| shim 二进制 | `which containerd-shim-runc-v2` | 存在 |
| shim 版本 | `containerd-shim-runc-v2 --version` | 与 containerd 匹配 |
| shim 进程数 | `ps aux | grep shim | wc -l` | ≈ 容器数 |
| shim 内存 | `cat /proc/<pid>/status | grep VmRSS` | < 50MB |
| 僵尸进程 | `ps -eo stat | grep Z` | 无 |
| containerd 重启 | `systemctl restart containerd` | 容器不受影响 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| shim v1 | containerd 1.0-1.3 | 每容器一个进程，gRPC |
| shim v2 | containerd 1.4+ | ttrpc，更高效 |
| shim v2 (2.0) | containerd 2.0+ | 新插件接口 |

## 架构对比

```text
Shim v1 vs v2 对比：

Shim v1 (gRPC):
  containerd → gRPC → shim (per-container)
  缺点：gRPC 开销大，每容器一个监听器

Shim v2 (ttrpc):
  containerd → ttrpc → shim (per-container)
  优点：轻量级协议，更少资源占用

多运行时 shim：
  containerd
    ├── shim-runc-v2 → runc → 容器
    ├── shim-kata-v2 → kata → microVM
    └── shim-runsc-v1 → runsc → gVisor
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 小集群 | 默认 | 足够 |
| 大集群 | 监控 shim 进程数 | 避免泄漏 |
| 多运行时 | 多个 shim 二进制 | 隔离 |
| 高密度 | 监控内存 | 每容器 5-15MB |

## 检查清单（补充）

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| ttrpc 连接 | `ls /run/containerd/` | socket 存在 |
| shim 日志 | `journalctl -u containerd` | 无 shim 错误 |
| 容器重启 | `systemctl restart containerd` | 容器不受影响 |
| 多运行时 | `crictl info` | 包含多个 runtime |

## 常见问题 FAQ（补充）

| 问题 | 解答 |
|------|------|
| shim v2 与 v1 区别？ | v2 使用 ttrpc 替代 gRPC，每个容器一个 shim 进程，资源开销更低 |
| shim 崩溃会影响容器吗？ | 不会，shim 独立于 containerd，重启后自动重连 |
| 如何查看 shim 进程？ | `ps aux | grep containerd-shim` |
| ttrpc 与 gRPC 区别？ | ttrpc 更轻量，无 HTTP/2 开销，适合本地通信 |
| 如何自定义 shim？ | 实现 ttrpc service 接口，注册为 containerd 插件 |
| shim 日志在哪？ | `/run/containerd/io.containerd.runtime.v2.task/` 下 |
| 多 shim 如何管理？ | containerd 自动管理生命周期，无需手动干预 |
| shim 资源占用多少？ | 每个 shim 约 5-10MB 内存，远低于 v1 |

## 性能调优参数

| 参数 | 默认值 | 生产建议 | 说明 |
|------|--------|----------|------|
| `shim_cgroup` | 无 | 独立 cgroup | 限制 shim 资源 |
| `io_uid` | 0 | 按需 | I/O 管道用户 |
| `io_gid` | 0 | 按需 | I/O 管道组 |
| `binary_name` | containerd-shim-runc-v2 | 按运行时 | shim 二进制路径 |

## 相关文档

- [[14-容器运行时/03-containerd-CRI-O/12-container-runtime-lifecycle.md|容器运行时生命周期]]
- [[14-容器运行时/03-containerd-CRI-O/11-cri-interface-internals.md|CRI 接口内部]]
- [[14-容器运行时/03-containerd-CRI-O/10-containerd-configuration-deep-guide.md|containerd 配置深度指南]]

<!-- risk-assessed -->
