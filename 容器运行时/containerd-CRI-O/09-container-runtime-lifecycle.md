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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| Pod 卡在 Terminating | 容器进程未响应 SIGTERM | `crictl inspect <id>` | 检查 preStop hook 和 grace period |
| 沙箱创建失败 | pause 镜像拉取失败 | `crictl pods` | 确认 pause 镜像在本地或内网可达 |
| 容器反复重启 | OOMKilled 或健康检查失败 | `kubectl describe pod <name>` | 调整资源限制或探针配置 |
| 网络命名空间泄漏 | 沙箱删除失败 | `ip netns list` | 手动清理残留 netns |
| 镜像 GC 不触发 | kubelet 阈值配置不当 | `kubelet --image-gc-high-threshold` | 调整 GC 阈值参数 |
| 容器启动慢 | 镜像层过多或磁盘 I/O 高 | `crictl pull <image> -v` | 优化镜像层数，检查磁盘性能 |
| cgroup 清理失败 | 进程残留 | `ls /sys/fs/cgroup/.../` | 手动清理 cgroup 目录 |
| 节点 NotReady | containerd 服务异常 | `systemctl status containerd` | 重启 containerd 并检查日志 |

## 容器生命周期状态机

```text
容器完整生命周期：

Created → Running → Exited → Removed
   │         │          │
   │         │          └── kubelet GC 清理
   │         └── OOMKilled / SIGTERM / SIGKILL
   └── 镜像拉取 + 沙箱创建 + 网络配置

Pod 沙箱生命周期：
RunPodSandbox → Ready → StopPodSandbox → RemovePodSandbox
                    │
                    └── 网络插件配置 CNI ADD
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 优雅停机 | 配置 preStop hook + SIGTERM | 给应用足够时间清理 |
| grace period | 设置合理的 terminationGracePeriodSeconds | 默认 30s，有状态服务调大 |
| pause 镜像 | 使用内网镜像仓库 | 避免外网拉取阻塞 |
| GC 策略 | 配置 kubelet 镜像 GC 阈值 | 与磁盘容量匹配 |
| CNI | 确保节点 CNI 二进制齐全 | 版本与 K8s 匹配 |
| 监控 | 监控容器重启次数 | 频繁重启及时告警 |
| 资源 | 设置 requests/limits | 避免资源争抢 |
| 探针 | 配置 liveness + readiness | 确保流量只到健康实例 |

## 相关工具

| 工具 | 用途 | 使用方式 |
|------|------|----------|
| crictl | 容器生命周期调试 | `crictl ps/pods/inspect` |
| kubectl | Pod 生命周期管理 | `kubectl get/describe/delete pod` |
| journalctl | 运行时日志 | `journalctl -u containerd -f` |
| ip netns | 网络命名空间管理 | `ip netns list/exec` |
| nsenter | 进入容器命名空间 | `nsenter -t <pid> -m -n` |
| cgroupfs | cgroup 管理 | 直接操作 /sys/fs/cgroup |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 容器和 Pod 的生命周期关系？ | Pod 包含沙箱 + 多个容器，沙箱先创建后销毁 |
| SIGTERM 和 SIGKILL 的区别？ | SIGTERM 可捕获优雅退出，SIGKILL 强制杀死不可捕获 |
| 为什么容器删除后还有残留？ | cgroup/netns 清理失败，需手动清理 |
| 如何强制删除卡住的 Pod？ | `kubectl delete pod --force --grace-period=0` |
| pause 容器的作用？ | 持有网络命名空间，作为 Pod 基础设施 |
| 镜像 GC 何时触发？ | 磁盘使用超过 image-gc-high-threshold 时 |
| 如何查看容器实际状态？ | `crictl inspect <id>` 查看完整状态 |
| 容器重启次数如何查看？ | `kubectl get pod` 的 RESTARTS 列 |

## 生命周期配置示例

```yaml
# Pod 优雅停机配置
apiVersion: v1
kind: Pod
metadata:
  name: graceful-shutdown-example
spec:
  terminationGracePeriodSeconds: 60
  containers:
  - name: app
    image: myapp:latest
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 10"]
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 10
      periodSeconds: 5
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 3
    resources:
      requests:
        memory: "128Mi"
        cpu: "100m"
      limits:
        memory: "256Mi"
        cpu: "500m"
```

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| Pod 启动慢 | 预拉取镜像 | DaemonSet 预加载关键镜像 |
| 停机时间长 | 优化 preStop | 减少不必要的等待时间 |
| 容器频繁重启 | 调整探针 | 增大 initialDelaySeconds |
| 资源不足 | 调整 limits | 根据实际使用调整 |
| GC 不及时 | 调整阈值 | image-gc-high-threshold |
| 沙箱创建慢 | 本地 pause 镜像 | 避免外网拉取 |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| container_restart_total | 容器重启次数 | > 5/hour |
| pod_start_duration | Pod 启动耗时 | P99 > 30s |
| container_oom_kills | OOM 次数 | > 0 |
| sandbox_create_duration | 沙箱创建耗时 | P99 > 5s |
| image_gc_duration | 镜像 GC 耗时 | > 60s |
| running_pods | 运行中 Pod 数 | > 节点容量 90% |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 资源限制 | 必须设置 requests/limits | 避免资源耗尽 |
| 探针 | 配置 liveness + readiness | 确保流量只到健康实例 |
| 优雅停机 | preStop + SIGTERM | 避免连接中断 |
| 权限 | 非 root 运行 | 最小权限原则 |
| 网络 | NetworkPolicy 限制 | 避免横向移动 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| Docker | containerd | 安装 containerd→配置 CRI→迁移 |
| 无探针 | 有探针 | 添加 liveness + readiness |
| 无 preStop | 有 preStop | 添加优雅停机逻辑 |
| 固定 GC | 动态 GC | 根据磁盘调整阈值 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 容器状态 | `crictl ps` | 无异常状态 |
| Pod 状态 | `kubectl get pods` | 无 CrashLoopBackOff |
| 重启次数 | `kubectl get pods` RESTARTS | < 5 |
| 资源使用 | `kubectl top pods` | 在 limits 内 |
| 探针 | `kubectl describe pod` | 配置正确 |
| GC | `crictl images` | 无过多无用镜像 |
| 日志 | `journalctl -u containerd` | 无错误 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| CRI v1alpha2 | K8s 1.7 | 初始生命周期接口 |
| CRI v1 | K8s 1.27 | 稳定 API |
| 用户命名空间 | K8s 1.30 | 增强隔离 |
| 优雅停机改进 | K8s 1.29 | preStop 优化 |

## 架构对比

```text
容器生命周期架构：

kubectl delete pod
  └── API Server
       └── kubelet
            └── CRI: StopContainer(SIGTERM)
                 └── shim → 容器进程
                      ├── 捕获 SIGTERM
                      ├── 执行清理逻辑
                      └── 退出 (或 SIGKILL)
            └── CRI: RemoveContainer
            └── CNI: DEL (清理网络)
            └── CRI: StopPodSandbox
            └── CRI: RemovePodSandbox

GC 流程：
kubelet → 检查磁盘使用 → 超过阈值 → 删除无用镜像/容器
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 无状态服务 | grace=30s | 默认 |
| 有状态服务 | grace=60-120s | 数据落盘 |
| 大镜像 | image-gc-high=85 | 避免频繁 GC |
| 小磁盘 | image-gc-high=70 | 及时清理 |

## 检查清单（补充）

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 优雅停机 | `kubectl delete pod` + 观察 | 无连接中断 |
| GC | `crictl images` | 无过多无用镜像 |
| 磁盘 | `df -h /var/lib/containerd` | < 80% |
| 探针 | `kubectl describe pod` | 配置正确 |
| 重启 | `kubectl get pods` RESTARTS | < 5 |

## 相关文档

- [[容器运行时/containerd-CRI-O/08-cri-interface-internals.md|CRI 接口内部]]
- [[容器运行时/containerd-CRI-O/12-container-shim-v2.md|containerd-shim-runc-v2]]
- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]

<!-- risk-assessed -->
