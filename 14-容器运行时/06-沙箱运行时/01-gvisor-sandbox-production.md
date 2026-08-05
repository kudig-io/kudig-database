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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| 容器启动失败 | runsc 二进制缺失 | `which runsc` | 安装 gVisor 并配置 PATH |
| 系统调用不支持 | gVisor 未实现该 syscall | `runsc debug --strace` | 检查兼容性列表或切换运行时 |
| 性能下降明显 | 文件 I/O 密集 | `runsc --overlay2` | 启用 overlay 缓存 |
| 网络异常 | netstack 兼容性问题 | `runsc debug --network` | 检查网络配置或切换 hostnet |
| 内核版本不兼容 | 内核过旧 | `uname -r` | 升级到 4.15+ |
| 内存使用异常 | sentry 内存泄漏 | `runsc debug --profile=heap` | 升级到已修复版本 |
| GPU 不可用 | gVisor 不支持 GPU | `nvidia-smi` | GPU 工作负载用 runc |
| 文件权限问题 | gofer 进程异常 | `ps aux | grep gofer` | 检查 gofer 配置和权限 |

## gVisor 架构详解

```text
gVisor 架构层次：

应用容器进程
  └── Sentry（用户态内核）
       ├── 系统调用拦截和模拟
       ├── VFS（虚拟文件系统）
       ├── Netstack（用户态网络栈）
       └── 平台层（ptrace/KVM/systrap）
            └── Gofer（文件代理进程）
                 └── 宿主机文件系统
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 兼容性 | 业务应用先过 gVisor 兼容性测试 | 避免生产故障 |
| 节点池 | RuntimeClass + nodeSelector 隔离 | 专用节点运行 gVisor |
| 文件 I/O | 文件密集负载启用 overlay 缓存 | 显著提升性能 |
| 平台 | 生产使用 systrap 平台 | 性能和稳定性最佳 |
| 监控 | 监控 sentry 内存和 CPU | 异常及时告警 |
| 升级 | 随 containerd 一起升级 runsc | 保持版本一致 |
| 回滚 | 保留 runc 作为默认运行时 | 问题时可快速切回 |
| 测试 | 定期运行 gVisor 兼容性测试套件 | 确保新版本兼容 |

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| runsc | gVisor 运行时 | 随 gVisor 安装 |
| runsc debug | 调试工具 | `runsc debug --strace <id>` |
| containerd-shim-runsc-v1 | K8s 集成 shim | 随 gVisor 安装 |
| kubectl | RuntimeClass 管理 | `kubectl get runtimeclass` |
| crictl | 容器调试 | `crictl info` |
| gvisor-tap-vsock | 网络工具 | 随 gVisor 分发 |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| gVisor 支持哪些系统调用？ | 大部分 Linux syscall，查看官方兼容性列表 |
| 性能开销多大？ | CPU 密集 5-10%，I/O 密集 20-40% |
| 需要硬件虚拟化吗？ | 不需要，ptrace/systrap 模式无需 KVM |
| 如何查看不支持的 syscall？ | `runsc debug --strace` 查看 UNSUPPORTED |
| gVisor 和 Kata 如何选择？ | 无 KVM 环境选 gVisor，强隔离选 Kata |
| 如何调试 gVisor 容器？ | `runsc debug --strace` 或查看 sentry 日志 |
| 支持 GPU 吗？ | 不支持，GPU 工作负载用 runc |
| 如何升级 runsc？ | 下载新版本替换二进制，重启 containerd |

## gVisor 配置示例

```toml
# /etc/containerd/config.toml - gVisor 运行时配置
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
  runtime_type = "io.containerd.runsc.v1"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options]
    TypeUrl = "io.containerd.runsc.v1.options"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options.config]
      # 使用 systrap 平台（推荐）
      platform = "systrap"
      # 启用 overlay 缓存（文件 I/O 密集场景）
      overlay2 = "all"
      # 网络配置
      network = "sandbox"
      # 调试模式（仅排障用）
      debug = false
      strace = false
```

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| 文件 I/O 慢 | overlay 缓存 | 配置 overlay2 = "all" |
| 网络延迟 | 网络模式 | 评估 hostnet vs sandbox |
| CPU 密集 | 平台选择 | 使用 systrap 平台 |
| 启动慢 | 预热 | 节点初始化时预加载 runsc |
| 内存占用 | 监控 sentry | 检查内存泄漏 |
| 兼容性 | 测试套件 | 运行前过兼容性测试 |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| sentry_memory_bytes | Sentry 内存 | > 512MB |
| sentry_cpu_seconds | Sentry CPU | 持续 > 80% |
| gofer_requests | Gofer 请求数 | 异常增长 |
| syscall_unsupported | 不支持的 syscall | > 0 |
| container_start_duration | 启动耗时 | P99 > 1s |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 平台 | 使用 systrap | 比 ptrace 更安全 |
| 网络 | sandbox 模式 | 完全隔离网络栈 |
| 文件 | overlay 只读 | 防止容器内修改 |
| 升级 | 及时更新 runsc | 修复已知漏洞 |
| 测试 | 定期兼容性测试 | 确保新版本兼容 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| runc | gVisor | 安装 runsc→配置 containerd→RuntimeClass |
| ptrace | systrap | 修改配置 platform = "systrap" |
| 无 overlay | overlay | 配置 overlay2 = "all" |
| 单节点 | 多节点 | 配置专用节点池 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| runsc 二进制 | `which runsc` | 存在 |
| runsc 版本 | `runsc --version` | 最新稳定版 |
| shim | `which containerd-shim-runsc-v1` | 存在 |
| RuntimeClass | `kubectl get runtimeclass gvisor` | 存在 |
| Pod 生效 | `kubectl get pod -o jsonpath='{.status.runtimeHandler}'` | runsc |
| 兼容性 | 运行测试套件 | 通过 |
| 性能 | 基准测试 | 在预期范围 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| gVisor 2018 | 2018 | 初始发布 |
| systrap 平台 | 2023 | 替代 ptrace，性能提升 |
| overlay2 | 2022 | 文件 I/O 优化 |
| netstack 改进 | 2024 | 网络性能提升 |

## 架构对比

```text
gVisor vs runc vs Kata：

runc:
  容器进程 → 宿主机内核 (共享)
  隔离：namespace + cgroup

gVisor:
  容器进程 → Sentry (用户态内核) → 宿主机内核
  隔离：系统调用拦截

Kata:
  容器进程 → Guest 内核 → Hypervisor → 宿主机
  隔离：硬件虚拟化
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 多租户 | systrap + overlay | 性能 + 安全 |
| 文件密集 | overlay2=all | I/O 优化 |
| 网络密集 | 评估 hostnet | 性能权衡 |
| 通用 | 默认配置 | 足够 |

## 相关文档

- [[14-容器运行时/05-运行时迁移/03-runtime-class-configuration.md|RuntimeClass 配置]]
- [[14-容器运行时/06-沙箱运行时/02-firecracker-microvm-guide.md|Firecracker microVM]]
- [[14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container.md|Kata Containers]]
- [[14-容器运行时/03-containerd-CRI-O/09-runtime-security-hardening.md|运行时安全加固]]

<!-- risk-assessed -->
