---
title: gVisor 沙箱运行时
description: 'gVisor (runsc) 系统调用拦截机制、安全模型、containerd/K8s 集成与性能基准完整指南'
summary: 'gVisor (runsc) 系统调用拦截机制、安全模型、containerd/K8s 集成与性能基准完整指南'
category: container-runtime
tags:
- gvisor
- runsc
- sandbox
- security
- syscall-interception
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- gVisor 是什么
- 如何配置 gVisor 与 containerd 集成
- gVisor 兼容性限制有哪些
trigger_keywords:
- gvisor
- runsc
- sandbox
- syscall
- security-runtime
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# gVisor 沙箱运行时

## 1. 架构概述

gVisor 是 Google 开发的应用内核，实现了 Linux 内核的主要接口，但运行在用户空间。它通过拦截容器的系统调用，在应用和宿主机内核之间插入一个安全边界，提供比传统容器更强的隔离。

### 1.1 核心组件

```
┌─────────────────────────────────────────────┐
│               Container                      │
│  ┌─────────────────────────────────────┐    │
│  │           Application                │    │
│  │        (系统调用请求)                │    │
│  └──────────────┬──────────────────────┘    │
│                 │ syscall                     │
│  ┌──────────────▼──────────────────────┐    │
│  │           Sentry (Go)                │    │
│  │  ┌──────────────────────────────┐   │    │
│  │  │  Linux 内核接口实现          │   │    │
│  │  │  (进程/文件系统/网络)        │   │    │
│  │  └──────────────────────────────┘   │    │
│  └──────────────┬──────────────────────┘    │
│                 │ 9P 协议                     │
│  ┌──────────────▼──────────────────────┐    │
│  │           Gofer (Go)                 │    │
│  │  (文件系统代理，受限权限)            │    │
│  └──────────────┬──────────────────────┘    │
│                 │                             │
└─────────────────┼───────────────────────────┘
                  │ 有限系统调用
┌─────────────────▼───────────────────────────┐
│              宿主机内核                       │
│  (gVisor 仅使用约 70 个系统调用)              │
└─────────────────────────────────────────────┘
```

### 1.2 Sentry 与 Gofer 职责

| 组件 | 职责 | 特点 |
|------|------|------|
| **Sentry** | 拦截并实现系统调用 | 用户空间 Linux 内核 |
| **Gofer** | 文件系统访问代理 | 最小权限，独立进程 |
| **runsc** | OCI 运行时入口 | 管理 Sentry 和 Gofer |

### 1.3 系统调用拦截机制

```
应用调用 open("/tmp/file", O_RDONLY)
    │
    ▼
Sentry 拦截 → 实现 open 逻辑
    │
    ▼
Sentry 通过 9P 协议请求 Gofer
    │
    ▼
Gofer 验证路径 → 访问真实文件系统
    │
    ▼
返回文件描述符给 Sentry → 返回给应用
```

## 2. 安装配置

### 2.1 安装 gVisor

```bash
# 方式 1：使用官方安装脚本
curl -fsSL https://gvisor.dev/archive.key | sudo gpg --dearmor -o /usr/share/keyrings/gvisor-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/gvisor-archive-keyring.gpg] https://storage.googleapis.com/gvisor/releases release main" | sudo tee /etc/apt/sources.list.d/gvisor.list > /dev/null
sudo apt-get update && sudo apt-get install -y runsc

# 方式 2：直接下载二进制
wget https://storage.googleapis.com/gvisor/releases/release/latest/x86_64/runsc
chmod +x runsc
sudo mv runsc /usr/local/bin/

# 验证安装
runsc --version
```

### 2.2 containerd 集成

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
    # 普通运行时
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"

    # gVisor 运行时
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.gvisor]
      runtime_type = "io.containerd.runsc.v1"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.gvisor.options]
        TypeUrl = "io.containerd.runsc.v1.options"
        BinaryName = "/usr/local/bin/runsc"
        # 可选：Root 目录
        Root = "/run/containerd/runsc"
```

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 重启 containerd
sudo systemctl restart containerd

# 验证 gVisor 运行时
crictl info | jq '.config.containerd.runtimes.gvisor'
```
### 2.3 Kubernetes RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: gvisor
overhead:
  podFixed:
    memory: "10Mi"
    cpu: "100m"
scheduling:
  nodeSelector:
    gvisor-runtime: "true"
```

### 2.4 节点标签与部署

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 标签支持 gVisor 的节点
kubectl label nodes node-1 gvisor-runtime=true
kubectl label nodes node-2 gvisor-runtime=true

# 测试 gVisor Pod
kubectl run gvisor-test --image=nginx --restart=Never \
  --overrides='{"spec":{"runtimeClassName":"gvisor"}}'

# 验证 gVisor 环境
kubectl exec gvisor-test -- dmesg | head -5
# 预期输出包含 "Starting gVisor"

kubectl exec gvisor-test -- cat /proc/version
# 预期输出包含 "gVisor"
```
## 3. 安全模型

### 3.1 安全边界对比

```
传统容器 (runc):
┌────────────────────────┐
│     应用进程            │
│         │               │
│    直接系统调用          │
│         │               │
│     宿主机内核          │ ← 攻击面大
└────────────────────────┘

gVisor:
┌────────────────────────┐
│     应用进程            │
│         │               │
│    gVisor 拦截          │ ← 用户空间内核
│    (Sentry)             │
│         │               │
│    有限系统调用          │ ← 攻击面小
│         │               │
│     宿主机内核          │
└────────────────────────┘
```

### 3.2 gVisor 支持的系统调用

gVisor 实现了约 380 个系统调用中的约 250 个，覆盖大部分常见操作：

```bash
# 查看 gVisor 支持的系统调用
runsc --list-syscalls

# 常见支持的系统调用
# 文件操作: open, read, write, close, stat, fstat, lstat, poll, lseek
# 进程管理: fork, exec, wait, exit, kill, getpid
# 网络操作: socket, bind, connect, listen, accept, sendto, recvfrom
# 内存管理: mmap, munmap, mprotect, brk
# 信号处理: signal, sigaction, sigprocmask

# 不完全支持的系统调用
# io_uring (部分支持)
# perf_event_open (不支持)
# ptrace (有限支持)
# bpf (不支持)
```

### 3.3 安全配置

```yaml
# 安全强化的 gVisor Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  runtimeClassName: gvisor
  containers:
  - name: app
    image: my-app:latest
    securityContext:
      # 基础安全
      privileged: false
      runAsNonRoot: true
      runAsUser: 1000
      readOnlyRootFilesystem: true
      allowPrivilegeEscalation: false

      # 禁用所有能力
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE  # 仅添加必要能力

      # Seccomp 配置
      seccompProfile:
        type: RuntimeDefault

      # AppArmor
      appArmorProfile:
        type: RuntimeDefault

    # 只读挂载
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /app/cache

  volumes:
  - name: tmp
    emptyDir:
      medium: Memory
      sizeLimit: "100Mi"
  - name: cache
    emptyDir:
      sizeLimit: "500Mi"
```

## 4. 兼容性限制

### 4.1 已知限制

| 特性 | 支持状态 | 说明 |
|------|---------|------|
| `io_uring` | 部分 | 新版本逐步支持 |
| `ptrace` | 有限 | 调试工具受限 |
| `bpf` | 不支持 | eBPF 程序无法运行 |
| `perf_event_open` | 不支持 | 性能分析工具受限 |
| NFS 挂载 | 不支持 | 使用其他存储方案 |
| 设备直通 | 不支持 | `/dev` 访问受限 |
| AppArmor | 部分 | 使用 gVisor 自己的策略 |
| SELinux | 不支持 | 使用 gVisor 自己的安全模型 |

### 4.2 不兼容的工作负载

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 不适合 gVisor 的场景
# 1. 需要 eBPF 的应用（Cilium、Falco）
# 2. 需要内核模块的应用
# 3. 需要 ptrace 的调试工具
# 4. 需要设备直通的应用（GPU、DPDK）
# 5. NFS 客户端

# 验证兼容性
kubectl run compat-test --image=my-app --restart=Never \
  --overrides='{"spec":{"runtimeClassName":"gvisor"}}'
kubectl logs compat-test
kubectl describe pod compat-test
```
### 4.3 常见问题排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 问题 1：应用启动失败
# 检查 gVisor 日志
kubectl logs <pod-name> -c <container-name>

# 问题 2：系统调用不支持
# 查看 Sentry 日志
kubectl exec <pod-name> -- cat /var/log/gvisor.log 2>/dev/null

# 问题 3：网络不通
# 检查网络配置
kubectl exec <pod-name> -- ip addr
kubectl exec <pod-name> -- ping -c 3 8.8.8.8

# 问题 4：文件权限错误
# 检查 Gofer 配置
# 确保 readOnlyRootFilesystem 配合 emptyDir 使用
```
## 5. 性能基准

### 5.1 启动性能

| 指标 | runc | gVisor | 差异 |
|------|------|--------|------|
| 冷启动 | ~200ms | ~300ms | +50% |
| 热启动 | ~100ms | ~150ms | +50% |
| 内存开销 | ~10MB | ~15MB | +50% |
| CPU 开销 | 基准 | +5-10% | 轻微 |

### 5.2 I/O 性能

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 文件 I/O 测试
kubectl run fio-gvisor --image=fio/fio --restart=Never \
  --overrides='{"spec":{"runtimeClassName":"gvisor","containers":[{"name":"fio-gvisor","image":"fio/fio","args":["--name=test","--rw=randread","--bs=4k","--size=1G","--runtime=30"]}]}}'
```
| I/O 模式 | runc | gVisor | 差异 |
|----------|------|--------|------|
| 顺序读 | 基准 | -20-30% | 中等 |
| 随机读 | 基准 | -30-40% | 明显 |
| 顺序写 | 基准 | -15-25% | 中等 |
| 网络吞吐 | 基准 | -10-15% | 轻微 |

### 5.3 优化建议

```yaml
# 性能优化的 gVisor Pod
apiVersion: v1
kind: Pod
metadata:
  name: optimized-app
  annotations:
    # gVisor 特定优化
    io.gvisor.log.level: "warning"
spec:
  runtimeClassName: gvisor
  containers:
  - name: app
    image: my-app:latest
    resources:
      requests:
        cpu: "500m"
        memory: "256Mi"
      limits:
        cpu: "2"
        memory: "1Gi"
    # 使用内存文件系统减少 I/O 开销
    volumeMounts:
    - name: tmp
      mountPath: /tmp
  volumes:
  - name: tmp
    emptyDir:
      medium: Memory
```

## 6. 与 Kata Containers 对比

| 特性 | gVisor | Kata Containers |
|------|--------|-----------------|
| 隔离级别 | 用户空间内核 | 硬件 VM |
| 启动速度 | 快（~300ms） | 慢（~1.5s） |
| 内存开销 | 低（~15MB） | 高（~130MB） |
| 兼容性 | 部分系统调用不支持 | 完全兼容 |
| 安全性 | 高 | 更高 |
| 推荐场景 | 多租户 SaaS | 高安全要求 |

## 7. 生产最佳实践

| 实践 | 建议 |
|------|------|
| 节点规划 | gVisor 节点单独标签 |
| 资源预留 | 预留 10% CPU 和 50MB 内存 |
| 监控 | 监控系统调用失败率 |
| 测试 | 先在测试环境验证兼容性 |
| 回退 | 保留 runc 作为回退方案 |
| 安全 | 结合 NetworkPolicy 和 PodSecurityPolicy |

## Related

- [[14-容器运行时/03-containerd-CRI-O/04-kata-containers-secure-container|Kata Containers 安全容器]]
- [[14-容器运行时/03-containerd-CRI-O/06-rootless-containers-guide|Rootless 容器指南]]

## See Also

- [gVisor 官方文档](https://gvisor.dev/docs/)
- [gVisor GitHub](https://github.com/google/gvisor)


<!-- risk-assessed -->
