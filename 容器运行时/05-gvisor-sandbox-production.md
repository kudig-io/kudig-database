---
title: gVisor 沙箱运行时生产指南
description: gVisor（runsc）用户态内核在生产环境的部署、RuntimeClass 接入、性能调优与故障排查
summary: gVisor（runsc）用户态内核在生产环境的部署、RuntimeClass 接入、性能调优与故障排查
category: container-runtime
tags:
- containerd
- cri
- runtime
- gvisor
- runsc
- sandbox
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

# gVisor 沙箱运行时生产指南

## 概述

gVisor 是 Google 开发的用户态内核，以 `runsc` 作为 OCI runtime 接入 containerd/CRI-O。它拦截容器的系统调用并在用户态实现（而非直通宿主内核），为不受信任工作负载、SaaS 多租、CI 跑用户代码等场景提供"第二道防线"，避免容器逃逸直接拿到宿主内核控制权。

## 隔离模型对比

| 运行时 | 内核共享 | 隔离层 | 性能损耗 | 启动延迟 |
|---|---|---|---|---|
| runc | 共享宿主内核 | namespace/cgroup | 无 | 极低 |
| gVisor (runsc) | 用户态内核 | Sentry + Gofer | 10-30% | 低 |
| Kata | 独立 VM 内核 | 轻量虚拟机 | 中 | 较高 |
| Firecracker | microVM | KVM | 低-中 | 极低 |

gVisor 在"隔离强度高于 runc、但比 VM 轻"的中间地带。

## 安装 runsc

``` bash
# 🟢 只读/安装
RUNSC_VERSION=20240729.0
curl -sL https://storage.googleapis.com/gvisor/releases/release/${RUNSC_VERSION}/x86_64/runsc \
  | sudo tee /usr/local/bin/runsc >/dev/null
sudo chmod +x /usr/local/bin/runsc
runsc --version
```

## containerd 接入

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
  runtime_type = "io.containerd.runsc.v1"
```

> ⚠️ **🟠 高危操作** — 重启 containerd 影响节点容器

``` bash
# 🔴 高风险：变更窗口
sudo containerd config dump >/dev/null && sudo systemctl restart containerd
# 验证 handler 已注册
crictl info | jq '.config.containerd.runtimes | keys'
```

## RuntimeClass 与 Pod 绑定

``` yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
scheduling:
  nodeSelector:
    sandbox-runtime: gvisor
---
apiVersion: v1
kind: Pod
metadata: { name: untrusted }
spec:
  runtimeClassName: gvisor
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/demo/runner:v1
```

``` bash
# 🟢 只读：确认 Pod 跑在 gVisor 内
kubectl get pod untrusted -o jsonpath='{.status.runtimeHandler}'
# Pod 内验证：/proc 不再是真实宿主内核视图
kubectl exec untrusted -- dmesg   # 通常失败或为空
```

## gVisor 架构：Sentry 与 Gofer

```
容器进程 ─ syscalls ─> Sentry（用户态内核，实现大部分 syscall）
                         │ 文件系统操作
                         ▼
                       Gofer（宿主侧文件系统代理，通过 9p）
```

- **Sentry**：拦截并模拟系统调用，不直通宿主内核
- **Gofer**：代理文件 I/O，容器文件实际来自宿主但经 9p 协议

## 兼容性与不支持项

gVisor 不支持/受限的特性：

| 特性 | 支持情况 |
|---|---|
| 绝大多数 Linux syscall | 支持 |
| 原始套接字 / 某些 netlink | 不支持 |
| 需 `CAP_SYS_ADMIN` 的内核操作 | 受限 |
| 加载内核模块 | 不支持 |
| `/proc`、`/sys` 部分 | 模拟/精简 |

应用若依赖底层内核特性（如 eBPF、iptables NAT 细节、perf），在 gVisor 下会异常，需先做兼容性测试。

## 性能调优

```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options]
  Type = "overlay"
  # 文件访问缓存，降低 Gofer 9p 开销
  Overlay = "all"
  # 平台（hostio/network 模式）
  Network = "runsc"
```

- 文件密集型：开 overlay 缓存减少 9p 往返
- 网络：默认 `runsc`（用户态 netstack，隔离强但慢），性能敏感可用 `host`（隔离弱）
- CPU 密集：gVisor 对纯计算损耗小，对系统调用密集（如大量小 I/O）损耗大

## 典型故障

| 玟象 | 根因 | 处理 |
|---|---|---|
| `runsc: operation not permitted` | 内核版本/capability 不足 | kernel ≥ 5.4，runsc 最新版 |
| 应用启动即崩 | 依赖不支持的 syscall | 查 `runsc` 日志，换 runc 或调整 |
| 文件读写极慢 | 9p 无 overlay 缓存 | 开 `Overlay=all` |
| Pod Pending | 节点无 runsc | 打 `sandbox-runtime=gvisor` 标签到专用池 |

``` bash
# 🟢 只读：开启 runsc 调试日志
sudo runsc --debug --strace spec
# 日志位置
ls /var/run/runsc/<container-id>/
```

## 适用场景

- **代码执行平台**（在线 IDE、CI 跑 PR 代码）
- **SaaS 多租**：租户进程隔离
- **不受信任镜像**：第三方镜像沙箱执行
- **不适用**：高性能网络/存储、依赖内核特性（DPDK/eBPF）的工作负载

## 生产检查清单

- [ ] runsc 版本与节点内核匹配（≥5.4 推荐）
- [ ] containerd 注册 `runsc` handler 并通过 `crictl info` 验证
- [ ] RuntimeClass 用 `nodeSelector` 隔离专用节点池
- [ ] 业务应用已过 gVisor 兼容性测试
- [ ] 文件密集负载启用 overlay 缓存

## 相关文档

- [[容器运行时/运行时迁移/02-runtime-class-configuration.md|RuntimeClass 配置]]
- [[容器运行时/06-firecracker-microvm-guide.md|Firecracker microVM]]
- [[容器运行时/containerd-CRI-O/04-kata-containers-secure-container.md|Kata Containers]]
- [[容器运行时/containerd-CRI-O/06-runtime-security-hardening.md|运行时安全加固]]

<!-- risk-assessed -->
