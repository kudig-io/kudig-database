---
title: containerd 容器生命周期与 Pod 管理
summary: containerd 容器生命周期与 Pod 管理：Created → Running → (Succeeded|Failed|Unknown)
  ↓ Paused → Running
category: synthesis
tags:
- synthesis
- containerd
- pod
- k8s
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd 容器生命周期与 Pod 管理

> containerd 如何实现 Kubernetes Pod 的容器生命周期管理。

## 概述

Kubernetes Pod 的生命周期最终由 containerd 落地实现。从 Pod 调度到节点开始，kubelet 通过 CRI 接口调用 containerd 创建 Pod Sandbox（pause 容器）、拉取镜像、创建容器进程、执行健康检查、直到最终优雅终止。理解这条完整链路对于排查容器启动失败、优雅停止、资源泄漏等问题至关重要。

## 生命周期映射

| Kubernetes 概念 | containerd 实现 | 底层技术 |
|----------------|----------------|----------|
| Pod Sandbox | pause 容器 | 独立的 network/IPC/UTS namespace |
| Init Container | 按序启动的容器 | 容器依赖链 |
| Main Container | 主业务容器 | runc/crun 创建 OCI bundle |
| Container Probe | 由 kubelet 调用 CRI Exec | ExecSync gRPC 调用 |
| Graceful Shutdown | SIGTERM → 等待 → SIGKILL | Linux signal + cgroup freeze |

## 容器状态转换

### 完整状态机

```
Created → Running → (Succeeded|Failed|Unknown)
              ↓
           Paused → Running       (checkpoint/restore)
              ↓
           Stopped → Deleted
```

| 状态 | 说明 | containerd 动作 |
|------|------|----------------|
| Created | 容器进程已创建但未启动 | runc create，namespace/cgroup 已设置 |
| Running | 容器进程正在运行 | runc start，shim 监控进程 |
| Paused | 容器进程被冻结 | cgroup freezer，用于 checkpoint |
| Stopped | 容器进程已退出 | shim 捕获 exit code |
| Succeeded | 退出码 = 0 | Job 完成标志 |
| Failed | 退出码 ≠ 0 | 异常退出，触发 restartPolicy |
| Unknown | kubelet 与 containerd 通信失败 | 超时标记，等待恢复 |

## Pod 创建流程详解

### CRI 调用序列

```
1. Pod Sandbox 创建
   kubelet → CRI RuntimeService.RunPodSandbox
   → containerd 创建 pause 容器
   → pause 容器持有:
     ├── Network namespace（由 CNI 配置）
     ├── IPC namespace
     └── UTS namespace
   → Pod 内所有容器共享这些 namespace

2. 镜像拉取（按需）
   kubelet → CRI ImageService.PullImage
   → containerd 检查 content store 是否已存在
   → 不存在: 从 registry 拉取，存储为 content blob
   → 创建 snapshot（overlayfs 层）

3. Init Container 按序执行
   对每个 init container:
   kubelet → CRI CreateContainer → containerd NewContainer
   kubelet → CRI StartContainer → containerd Start → runc
   → 等待 init container 退出（必须成功才继续下一个）

4. Main Container 创建与启动
   对每个 main container（并行）:
   kubelet → CRI CreateContainer → containerd NewContainer
     → 设置 rootfs（镜像层 + 可写层）
     → 设置 cgroup（CPU/Memory/BlockIO 限制）
     → 设置 environment/mounts/devices
   kubelet → CRI StartContainer → containerd Start
     → containerd-shim-runc-v2 → runc → 容器进程启动

5. 健康检查
   kubelet 定期调用:
   → CRI ExecSync (exec probe command)
   → 或 CRI ContainerStatus (检查 exit code)
```

## 关键操作

### 停止与终止流程

```
1. 停止（优雅终止）
   kubelet → CRI StopContainer
   → containerd Kill(SIGTERM)
   → 等待 terminationGracePeriodSeconds（默认 30s）
   → 超时后 containerd Kill(SIGKILL)
   → shim 记录 exit code

2. 删除
   kubelet → CRI RemoveContainer
   → containerd Delete
   → 清理 rootfs snapshot
   → 清理 cgroup
   → 清理日志文件
```

### 优雅终止最佳实践

```yaml
spec:
  terminationGracePeriodSeconds: 60    # 给应用 60s 优雅退出时间
  containers:
    - name: app
      lifecycle:
        preStop:                        # SIGTERM 前先执行
          exec:
            command: ["/bin/sh", "-c", "sleep 15 && nginx -s quit"]
      # 应用代码需处理 SIGTERM:
      # - 停止接收新请求
      # - 完成进行中的请求
      # - 清理资源（关闭连接、刷新缓冲）
      # - 退出进程
```

## 调试与排查

```bash
# 🟢 低风险：只读/信息收集
# 查看 containerd 管理的容器
crictl ps -a                          # 所有容器（包括已停止的）
crictl ps --state Running             # 仅运行中的

# 查看 Pod sandbox
crictl pods                           # 所有 Pod sandbox
crictl pods --name <pod-name>         # 按 Pod 名筛选

# 查看容器日志
crictl logs <container-id>            # 容器标准输出
crictl logs --previous <container-id> # 上一次容器的日志

# 检查容器详情（包括状态、资源限制、挂载）
crictl inspect <container-id>

# 在容器内执行命令
crictl exec -it <container-id> /bin/sh

# 🟡 中风险：调试操作
# 查看容器进程树（在节点上）
ps aux | grep <container-id>
# 查看 cgroup 信息
cat /sys/fs/cgroup/kubepods/<pod-uid>/<container-id>/cgroup.procs
```

## 常见问题诊断

| 问题 | 可能原因 | 诊断方法 |
|------|---------|---------|
| ContainerCreating 卡住 | 镜像拉取慢或 sandbox 创建失败 | `crictl pods` + `kubectl describe pod` |
| CrashLoopBackOff | 容器启动后立即退出 | `crictl logs --previous` 查看退出原因 |
| 优雅停止超时 | 应用未处理 SIGTERM | 检查 `terminationGracePeriodSeconds` 和 preStop |
| 容器僵尸进程 | PID 1 未回收子进程 | 检查容器 init 进程，使用 tini/dumb-init |

## 最佳实践

- **处理 SIGTERM 信号**：容器入口进程必须捕获 SIGTERM 并优雅退出，否则只能等待 SIGKILL 强制终止（可能丢失数据）
- **使用 preStop hook 争取缓冲时间**：Service endpoint 删除有传播延迟，`sleep 5-10s` 可确保不再有新流量到达正在终止的 Pod
- **合理设置 terminationGracePeriodSeconds**：有状态服务（如数据库连接池）需要更长的优雅退出时间（60-120s）
- **避免以 root 运行**：配合 User Namespaces（K8s 1.36 GA），容器内 root 映射到宿主机非特权用户
- **使用 tini/dumb-init 作为 PID 1**：避免僵尸进程问题，确保信号正确传递

## 常见陷阱

- **SIGTERM 被 PID 1 吞掉**：如果容器以 shell 脚本作为 PID 1，shell 不会将 SIGTERM 转发给子进程——使用 `exec` 替换进程或使用 tini
- **pause 容器被误删**：pause 容器管理 Pod 的网络命名空间，被删除后 Pod 内所有容器网络中断——不应手动操作 pause 容器
- **日志丢失**：容器删除后 `/var/log/pods/` 下的日志会被清理——需要配置日志采集（Fluent Bit）在日志产生时实时转发

## 相关页面

- [[containerd]] — containerd 详细文档
- [[概念/kubernetes-containerd-integration.md|K8s 与 containerd 集成]] — CRI 通信架构
- [[概念/etcd-containerd-storage.md|etcd 与 containerd 存储]] — 存储层机制
- [[pod-lifecycle]] — Pod 生命周期
- [[kubelet]] — 节点代理


<!-- risk-assessed -->
