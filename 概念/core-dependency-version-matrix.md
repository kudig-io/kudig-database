---
title: 核心依赖版本矩阵
description: etcd 是 Kubernetes 的唯一状态存储，版本选择直接影响集群的稳定性和性能。
summary: etcd 是 Kubernetes 的唯一状态存储，版本选择直接影响集群的稳定性和性能。
category: concepts
tags:
- k8s
- release-notes
- etcd
- containerd
- cri-o
- coredns
- runc
- docker
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 核心依赖版本矩阵 是什么
- 如何 核心依赖版本矩阵
trigger_keywords:
- 核心依赖版本矩阵
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 核心依赖版本矩阵

> 本文档综合了 `生态参考/_archived-release-notes/core-deps/` 目录下 5 个核心依赖项目的 83 个版本发布说明 ^[inferred]

## etcd 版本演进

etcd 是 Kubernetes 的唯一状态存储，版本选择直接影响集群的稳定性和性能。

| etcd 版本 | 关键特性 | 兼容 [[系统基础/速查卡/k8s.md|[[Kubernetes 生产环境速查卡|k8s]]]] 版本 |
|---|---|---|
| v3.0 | v3 API 引入、lease 机制、watch 改进 | v1.3 - v1.6 |
| v3.1 | 快照恢复改进、etcdctl v3 完善 | v1.6 - v1.8 |
| v3.2 | [[gRPC|gRPC]] proxy、etcd operator 支持、性能优化 | v1.8 - v1.12 |
| v3.3 | 改进的压缩机制、更好的监控指标 | v1.12 - v1.15 |
| v3.4 | 压缩优化、WAL 预写日志改进、etcd 嵌入 etcd | v1.15 - v1.18 |
| v3.5 | 多版本并发控制、改进的 leader 选举、更好的集群恢复 | v1.20+ |

### etcd v3.0 关键变更

- v3 API 成为核心：key-value 存储从 v2 转向 v3
- Lease 机制引入，支持 TTL 关联
- Snapshot restore 支持 lease key
- etcdctl v3 命令体系建立

## [[containerd|containerd]] 版本演进

containerd 从 Docker 中独立出来后成为 Kubernetes 的主要容器运行时。

| containerd 版本 | 关键特性 | 兼容 K8s 版本 |
|---|---|---|
| v1.0 | CRI 支持初版、基础容器生命周期管理 | v1.10 - v1.12 |
| v1.1 | CRI v1alpha1 改进、更好的 Windows 支持 | v1.10 - v1.13 |
| v1.2 | CRI v1 稳定、容器快照改进 | v1.13 - v1.15 |
| v1.3 | 改进的快照和迁移 | v1.15 - v1.18 |
| v1.4 | 支持 Kubernetes 1.20、CRI v1 完善 | v1.20 - v1.22 |
| v1.5 | 完整的 CRI v1、更好的 Pod 沙箱支持 | v1.22 - v1.24 |
| v1.6+ | 长期支持版本、全面的 CRI 实现 | v1.24+ |

### containerd v1.0 关键变更

- FIFO 死锁问题修复（healthcheck 相关）
- 快照 GC 修复
- 用户命名空间 mknod 处理
- 依赖 btrfs 更新

## [[cri-o|CRI-O]] 版本演进

CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，是 containerd 的替代方案。

| CRI-O 版本 | 对应 K8s 版本 | 关键特性 |
|---|---|---|
| v1.10 | v1.10 | CRI 基础实现 |
| v1.11 | v1.11 | 改进的容器生命周期 |
| v1.12 | v1.12 | Pod 沙箱优化 |
| v1.13 | v1.13 | SELinux 集成 |
| v1.14 - v1.20 | v1.14 - v1.20 | 逐步完善的 CRI 实现 |
| v1.21+ | v1.21+ | 现代化 CRI 运行时 |

## [[coredns|coredns]] 版本演进

CoreDNS 自 Kubernetes v1.11 起成为默认 DNS 服务。

| CoreDNS 版本 | 关键特性 |
|---|---|
| v010 - v0.99 | 早期开发版本，插件架构确立 |
| v1.0 | 首次 GA、Kubernetes 插件成熟 |
| v1.1 - v1.3 | 改进的 Kubernetes 服务发现 |
| v1.4 - v1.6 | 更好的性能、改进的缓存机制 |
| v1.7+ | 现代 DNS 服务、更好的监控集成 |

## runc 版本演进

runc 是 OCI 容器运行时的参考实现，被 containerd 和 CRI-O 底层使用。

| runc 版本 | 关键特性 |
|---|---|
| v0.1 - v0.4 | 早期 OCI 实现 |
| v0.5 - v0.9 | 改进的 cgroups 支持 |
| v1.0 | OCI 规范 v1.0 完全实现 |
| v1.1+ | 现代 cgroups v2 支持、安全加固 |

## 版本兼容性建议

### 推荐的 K8s + 核心依赖组合

| K8s 版本 | etcd | containerd | CoreDNS |
|---|---|---|---|
| v1.28 | v3.5.x | v1.6.x | v1.10.x |
| v1.29 | v3.5.x | v1.7.x | v1.11.x |
| v1.30 | v3.5.x | v1.7.x | v1.11.x |
| v1.31 | v3.5.x | v1.7.x | v1.11.x |
| v1.32 | v3.5.x | v1.7.x | v1.11.x |

## 升级注意事项

1. **etcd 备份**：升级 etcd 前务必备份数据
2. **containerd 兼容性**：确保 containerd 版本支持目标 K8s 版本的 CRI API
3. **CoreDNS 迁移**：从 kube-dns 迁移到 CoreDNS 需要规划
4. **runc 安全**：关注 runc 的 CVE 修复版本（如 CVE-2019-5736）

## 源码实现分析

### CRI 接口版本演进

```go
// k8s.io/cri-api/pkg/apis/runtime/v1/api.proto
// CRI v1 接口（K8s 1.26+ 默认，containerd 1.7+ 支持）
service RuntimeService {
    // Pod 沙箱生命周期
    rpc RunPodSandbox(RunPodSandboxRequest) returns (RunPodSandboxResponse);
    rpc StopPodSandbox(StopPodSandboxRequest) returns (StopPodSandboxResponse);
    rpc RemovePodSandbox(RemovePodSandboxRequest) returns (RemovePodSandboxResponse);
    // 容器生命周期
    rpc CreateContainer(CreateContainerRequest) returns (CreateContainerResponse);
    rpc StartContainer(StartContainerRequest) returns (StartContainerResponse);
    rpc StopContainer(StopContainerRequest) returns (StopContainerResponse);
}

// containerd 内部 CRI 插件实现
// github.com/containerd/containerd/pkg/cri/server/sandbox_run.go
func (c *criService) RunPodSandbox(ctx context.Context, r *runtime.RunPodSandboxRequest) {
    // 1. 创建 sandbox 容器（pause 容器）
    sandbox := c.createSandboxContainer(config)
    // 2. 设置网络命名空间（调用 CNI）
    c.setupPodNetwork(ctx, sandbox)
    // 3. 启动 sandbox
    task, _ := sandbox.NewTask(ctx, cio.NewCreator())
    task.Start(ctx)
}
```

### 版本依赖关系图

```
┌───────────────────────────────────────────────────────────┐
│          K8s 核心依赖版本关系                          │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  Kubernetes v1.32                                        │
│    ├─ etcd v3.5.x    ← 状态存储（必须先行升级）       │
│    ├─ containerd v1.7.x ← 容器运行时（CRI v1）        │
│    │    └─ runc v1.1.x  ← OCI 运行时（安全关键）     │
│    ├─ CoreDNS v1.11.x ← 集群 DNS                      │
│    └─ CNI plugins v1.4+ ← 网络插件                    │
│                                                           │
│  升级顺序（严格）:                                       │
│  etcd → containerd/runc → kubelet → apiserver → CoreDNS │
│                                                           │
│  禁止操作:                                               │
│  ✗ 跳过 etcd 小版本（如 3.4→3.6）                     │
│  ✗ containerd 降级（可能导致容器丢失）                │
│  ✗ kubelet 超过 apiserver 2 个小版本                  │
└───────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：升级前版本兼容性检查（🟢 只读）

```bash
# 检查当前集群各组件版本
kubectl get nodes -o wide  # kubelet 版本
kubectl exec -n kube-system etcd-master -- etcd --version
kubectl exec -n kube-system coredns-xxx -- coredns -version
crictl version  # containerd + runc 版本

# 检查 CRI 版本兼容性
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{": "}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'
```

### 场景二：etcd 升级前备份（🔴 关键操作）

```bash
# 🔴 升级前必须备份 etcd
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-pre-upgrade.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证备份完整性
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-pre-upgrade.db --write-out=table
```

### 场景三：containerd 升级流程（🔴 影响节点上所有容器）

```bash
# 1. 驱逐节点上所有 Pod
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 2. 升级 containerd
apt-get update && apt-get install -y containerd.io=1.7.*
systemctl restart containerd

# 3. 验证运行时状态
crictl info | jq '.config.containerd.runtimes'
crictl ps  # 确认容器恢复

# 4. 恢复调度
kubectl uncordon <node>
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| K8s 升级只需升级 apiserver | 必须同步升级 etcd/kubelet/CoreDNS/运行时 |
| etcd 可以跳版本升级 | etcd 必须逐小版本升级，跳版本会损坏数据 |
| containerd 升级不影响运行容器 | 重启 containerd 会短暂中断容器，必须先 drain |
| runc 版本无关紧要 | runc CVE 可直接逃逸容器，必须及时更新 |
| CoreDNS 升级无风险 | CoreDNS 配置不兼容会导致全集群 DNS 失败 |
| kubelet 可以比 apiserver 新 | kubelet 最多落后 apiserver 2 个小版本，不能超前 |

## 面试要点

1. **K8s 核心依赖的升级顺序是什么？**
   - etcd → containerd/runc → kubelet → kube-apiserver → CoreDNS
   - 原因：下层依赖必须先就绪，否则上层无法正常工作

2. **CRI v1alpha2 和 CRI v1 的区别？**
   - v1alpha2 在 K8s 1.26 废弃，v1 成为默认
   - containerd 1.7+ 和 CRI-O 1.26+ 支持 CRI v1
   - 主要变化：移除冗余字段、统一错误码

3. **runc CVE-2019-5736 的影响和修复？**
   - 容器内进程可覆盖宿主机 runc 二进制
   - 修复：升级 runc ≥1.0.0-rc6，使用只读 runc 挂载
   - 影响：所有使用 runc 的运行时（Docker/containerd/CRI-O）

4. **如何设计集群版本升级策略？**
   - 测试环境先行验证 → 生产滚动升级
   - 每次只升级一个小版本，不跳版
   - 升级前备份 etcd，升级后验证全组件健康

## 来源文档

- 生态参考/_archived-release-notes/core-deps/etcd/（15 个文件）
- 生态参考/_archived-release-notes/core-deps/containerd/（13 个文件）
- 生态参考/_archived-release-notes/core-deps/cri-o/（32 个文件）
- 生态参考/_archived-release-notes/core-deps/coredns/（16 个文件）
- 生态参考/_archived-release-notes/core-deps/runc/（7 个文件）

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[etcd]] — etcd


<!-- risk-assessed -->
