---
title: K8s 基础知识词典
description: 涵盖 Kubernetes 基础架构全领域的完整术语体系，包括控制平面、节点组件、容器运行时、核心对象、集群架构等
summary: K8s 基础领域词典，覆盖 kube-apiserver、etcd、kubelet、containerd、CRI、控制器模式、Namespace 等核心概念
category: dictionary
tags:
- dictionary
- fundamentals
- control-plane
- container-runtime
- kubernetes
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: beginner
audience:
- 开发工程师
- 平台工程师
- SRE
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# K8s 基础知识词典（Fundamentals）

> 本词典覆盖 Kubernetes 基础架构领域的核心术语、技术组件及工程实践，是理解 K8s 内部机制的权威参考。

## 领域概述

Kubernetes 基础架构是理解整个云原生生态的根基，包括：

- **控制平面**：kube-apiserver、etcd、scheduler、controller-manager
- **节点组件**：kubelet、kube-proxy、容器运行时
- **容器运行时**：containerd、CRI-O、runc、Kata
- **核心对象**：Pod、Namespace、Node、Label、Annotation
- **API 机制**：REST API、对象管理、Finalizer、GC
- **集群架构**：高可用、多节点、自愈机制

## 核心术语定义

### 控制平面组件

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| kube-apiserver | 集群 API 网关，所有请求入口 | REST、认证、授权、准入 |
| etcd | 分布式 KV 存储，集群状态存储 | Raft 共识、强一致 |
| kube-scheduler | Pod 调度决策 | 过滤+打分、可扩展 |
| kube-controller-manager | 控制器集合 | 循环协调、自愈 |
| cloud-controller-manager | 云平台集成控制器 | Node/Route/Service |
| Control Plane | 控制平面总称 | 集群大脑 |
| Master Node | 控制平面节点（已废弃术语） | 建议用 Control Plane Node |

### 节点组件

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| kubelet | 节点代理，管理 Pod 生命周期 | CRI 调用、探针、资源 |
| kube-proxy | Service 流量转发 | iptables/IPVS/eBPF |
| Node | 工作节点抽象 | 条件、污点、容量 |
| Worker Node | 运行工作负载的节点 | 与 Control Plane 分离 |
| Virtual Kubelet | 虚拟节点实现 | Serverless/Fargate |

### 容器运行时

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| CRI | 容器运行时接口 | kubelet ↔ runtime |
| containerd | 工业级容器运行时 | CNCF 毕业、轻量 |
| CRI-O | 轻量级 K8s 专用运行时 | Red Hat、无冗余 |
| runc | OCI 运行时参考实现 | 底层容器创建 |
| Kata Containers | 轻量级 VM 运行时 | 强隔离 |
| Kuasar | 华为多沙箱运行时 | 多运行时统一 |
| youki | Rust 实现的 OCI 运行时 | 内存安全 |
| urunc | 微 VM 运行时管理器 | Firecracker/Cloud Hypervisor |
| Docker | 容器平台（已移除 dockershim） | 开发用，生产用 containerd |
| WasmEdge | WebAssembly 运行时 | 轻量、安全 |
| container2wasm | 容器转 Wasm 工具 | 实验性 |
| Hyperlight | 微软微 VM 运行时 | 极致轻量 |

### 核心对象与概念

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Pod | 最小调度单元 | 共享网络/存储 |
| Namespace | 资源隔离边界 | 多租户、RBAC |
| Label | 键值对标签 | 选择器、分组 |
| Annotation | 元数据注解 | 工具信息、非选择 |
| Finalizer | 删除保护机制 | 清理外部资源 |
| Owner Reference | 所有者引用 | GC 级联删除 |
| UID | 对象唯一标识 | 不可变 |
| ResourceVersion | 乐观并发控制 | 冲突检测 |
| Field Selector | 字段过滤 | status.phase=Running |
| Lease | 租约对象 | 节点心跳、Leader 选举 |

### 集群架构与机制

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Controller Pattern | 控制器模式（期望状态→实际状态） | Reconcile Loop |
| Level-triggered | 水平触发（关注最终状态） | 而非边缘触发 |
| Self-healing | 自愈机制 | 自动重启/重调度 |
| Garbage Collection | 垃圾回收 | 级联删除、孤儿清理 |
| Storage Version | API 存储版本 | etcd 中的编码版本 |
| Mixed Version Proxy | 混合版本代理 | 滚动升级兼容 |
| Cgroup v2 | 统一 cgroup 层级 | 资源控制增强 |
| Flatcar | 不可变容器 OS | 自动更新 |
| Kairos | 不可变 Linux 框架 | 边缘/嵌入式 |
| bpfman | eBPF 程序管理器 | 生命周期管理 |
| CNCF | 云原生计算基金会 | 项目孵化/毕业 |

## 技术组件索引

### 控制平面

- [[17-系统基础/06-知识字典/fundamentals/kube-apiserver.md|kube-apiserver]]
- [[17-系统基础/06-知识字典/fundamentals/etcd.md|etcd]]
- [[17-系统基础/06-知识字典/fundamentals/kube-scheduler.md|kube-scheduler]]
- [[17-系统基础/06-知识字典/fundamentals/kube-controller-manager.md|kube-controller-manager]]
- [[17-系统基础/06-知识字典/fundamentals/controller-manager.md|Controller Manager]]
- [[17-系统基础/06-知识字典/fundamentals/cloud-controller-manager.md|Cloud Controller Manager]]
- [[17-系统基础/06-知识字典/fundamentals/control-plane.md|Control Plane]]
- [[17-系统基础/06-知识字典/fundamentals/control-plane-node.md|Control Plane Node]]
- [[17-系统基础/06-知识字典/fundamentals/master-node.md|Master Node]]

### 节点组件

- [[17-系统基础/06-知识字典/fundamentals/kubelet.md|kubelet]]
- [[17-系统基础/06-知识字典/fundamentals/kube-proxy.md|kube-proxy]]
- [[17-系统基础/06-知识字典/fundamentals/node.md|Node]]
- [[17-系统基础/06-知识字典/fundamentals/nodes.md|Nodes]]
- [[17-系统基础/06-知识字典/fundamentals/worker-node.md|Worker Node]]
- [[17-系统基础/06-知识字典/fundamentals/virtual-kubelet.md|Virtual Kubelet]]

### 容器运行时

- [[17-系统基础/06-知识字典/fundamentals/container-runtime.md|Container Runtime]]
- [[17-系统基础/06-知识字典/fundamentals/containerd.md|containerd]]
- [[17-系统基础/06-知识字典/fundamentals/cri-o.md|CRI-O]]
- [[17-系统基础/06-知识字典/fundamentals/cri.md|CRI]]
- [[17-系统基础/06-知识字典/fundamentals/runc.md|runc]]
- [[17-系统基础/06-知识字典/fundamentals/kata-containers.md|Kata Containers]]
- [[17-系统基础/06-知识字典/fundamentals/kuasar.md|Kuasar]]
- [[17-系统基础/06-知识字典/fundamentals/youki.md|youki]]
- [[17-系统基础/06-知识字典/fundamentals/urunc.md|urunc]]
- [[17-系统基础/06-知识字典/fundamentals/docker.md|Docker]]
- [[17-系统基础/06-知识字典/fundamentals/wasmedge.md|WasmEdge]]
- [[17-系统基础/06-知识字典/fundamentals/container2wasm.md|container2wasm]]
- [[17-系统基础/06-知识字典/fundamentals/hyperlight.md|Hyperlight]]

### 核心对象

- [[17-系统基础/06-知识字典/fundamentals/container.md|Container]]
- [[17-系统基础/06-知识字典/fundamentals/namespace.md|Namespace]]
- [[17-系统基础/06-知识字典/fundamentals/namespaces.md|Namespaces]]
- [[17-系统基础/06-知识字典/fundamentals/labels-and-selectors.md|Labels and Selectors]]
- [[17-系统基础/06-知识字典/fundamentals/annotations.md|Annotations]]
- [[17-系统基础/06-知识字典/fundamentals/finalizers.md|Finalizers]]
- [[17-系统基础/06-知识字典/fundamentals/owners-and-dependents.md|Owners and Dependents]]
- [[17-系统基础/06-知识字典/fundamentals/object-names-and-ids.md|Object Names and IDs]]
- [[17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes.md|Objects in Kubernetes]]
- [[17-系统基础/06-知识字典/fundamentals/field-selectors.md|Field Selectors]]
- [[17-系统基础/06-知识字典/fundamentals/recommended-labels.md|Recommended Labels]]
- [[17-系统基础/06-知识字典/fundamentals/leases.md|Leases]]

### 集群架构与机制

- [[17-系统基础/06-知识字典/fundamentals/kubernetes.md|Kubernetes]]
- [[17-系统基础/06-知识字典/fundamentals/cluster.md|Cluster]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-components.md|Kubernetes Components]]
- [[17-系统基础/06-知识字典/fundamentals/controllers.md|Controllers]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing.md|Self Healing]]
- [[17-系统基础/06-知识字典/fundamentals/garbage-collection.md|Garbage Collection]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-object-management.md|Object Management]]
- [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|The Kubernetes API]]
- [[17-系统基础/06-知识字典/fundamentals/the-kubectl-command-line-tool.md|kubectl]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-concepts-reference.md|Concepts Reference]]
- [[17-系统基础/06-知识字典/fundamentals/communication-between-nodes-and-the-control-plane.md|Node-Control Plane 通信]]
- [[17-系统基础/06-知识字典/fundamentals/storage-versions.md|Storage Versions]]
- [[17-系统基础/06-知识字典/fundamentals/mixed-version-proxy.md|Mixed Version Proxy]]
- [[17-系统基础/06-知识字典/fundamentals/about-cgroup-v2.md|Cgroup v2]]

### 操作系统与底层

- [[17-系统基础/06-知识字典/fundamentals/flatcar.md|Flatcar]]
- [[17-系统基础/06-知识字典/fundamentals/kairos.md|Kairos]]
- [[17-系统基础/06-知识字典/fundamentals/bpfman.md|bpfman]]
- [[17-系统基础/06-知识字典/fundamentals/cncf.md|CNCF]]

## 深度技术解析

### kube-apiserver 请求处理流程

```
客户端请求 → 认证(AuthN) → 授权(AuthZ) → 准入控制(Admission) → etcd 存储
     │            │            │              │              │
  TLS/mTLS   X.509/Token  RBAC/Webhook  Mutating/     写入/读取
  OIDC       ABAC         Node          Validating    Watch
```

**关键参数**：
- `--max-requests-inflight`: 最大并发请求数（默认 400）
- `--max-mutating-requests-inflight`: 最大写并发（默认 200）
- `--request-timeout`: 请求超时（默认 60s）
- `--watch-cache-sizes`: Watch 缓存大小

### etcd 运维要点

```bash
# 🟢 检查集群健康
etcdctl endpoint health --cluster

# 🟢 查看成员列表
etcdctl member list -w table

# 🟢 查看数据库大小
etcdctl endpoint status --write-out=table

# 🟡 压缩历史版本
etcdctl compact $(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')

# 🟡 碎片整理（会短暂阻塞）
etcdctl defrag --cluster

# 🔴 快照备份
etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db

# 🔴 快照恢复
etcdctl snapshot restore /backup/etcd.db --data-dir=/var/lib/etcd-restored
```

### kubelet 工作原理

kubelet 核心循环：
1. **Pod Sync Loop**：监听 API Server 的 Pod 变更
2. **PLEG**：Pod Lifecycle Event Generator，检测容器状态变化
3. **Probe Manager**：执行 Startup/Liveness/Readiness 探针
4. **Volume Manager**：挂载/卸载存储卷
5. **Status Manager**：上报 Pod/Node 状态

### 控制器模式（Reconcile Loop）

```go
// 控制器核心逻辑伪代码
func (c *Controller) Reconcile(key string) error {
    // 1. 获取期望状态（从 API Server）
    desired := c.getDesiredState(key)
    // 2. 获取实际状态（从集群/外部系统）
    current := c.getCurrentState(key)
    // 3. 计算差异并执行协调
    if !reflect.DeepEqual(desired, current) {
        c.takeAction(desired, current)
    }
    return nil
}
// 触发条件：事件驱动（Watch）+ 定期重同步（resyncPeriod）
```

### 高可用架构

生产环境 K8s 高可用架构：

```
                    ┌─────────────┐
                    │   VIP/LB    │
                    └──────┬──────┘
           ┌───────────┼───────────┐
           │               │               │
    ┌──────┴──────┐ ┌──────┴──────┐ ┌──────┴──────┐
    │ CP Node 1   │ │ CP Node 2   │ │ CP Node 3   │
    │ apiserver   │ │ apiserver   │ │ apiserver   │
    │ scheduler   │ │ scheduler   │ │ scheduler   │
    │ ctrl-mgr    │ │ ctrl-mgr    │ │ ctrl-mgr    │
    │ etcd        │ │ etcd        │ │ etcd        │
    └─────────────┘ └─────────────┘ └─────────────┘
```

**高可用要点**：
- etcd: 3/5 节点（奇数），跨 AZ 部署
- API Server: 无状态，多副本 + LB
- Scheduler/Controller-Manager: Leader 选举，多副本热备
- kubelet: 每节点一个，通过 Lease 上报心跳

## 生产案例

### 案例 1：etcd 磁盘延迟导致集群不可用

**现象**：API Server 响应超时，kubectl 命令卡住

**根因**：etcd 磁盘 IOPS 不足，fsync 延迟 > 10ms

**解决**：
```bash
# 检查 etcd 磁盘延迟
etcdctl endpoint status --write-out=table
# 监控指标
etcd_disk_wal_fsync_duration_seconds
etcd_disk_backend_commit_duration_seconds
# 解决：使用 SSD/NVMe，分离 etcd 磁盘
```

### 案例 2：kubelet 证书过期导致节点 NotReady

**现象**：多个节点状态变为 NotReady

**根因**：kubelet 客户端证书过期（默认 1 年），未启用自动轮换

**解决**：
```bash
# 检查证书过期时间
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
# 启用证书轮换
# kubelet: --rotate-certificates=true
# controller-manager: --cluster-signing-duration=8760h
```

### 案例 3：Finalizer 阻塞 Namespace 删除

**现象**：Namespace 删除卡在 Terminating 状态数小时

**根因**：Namespace 内资源的 Finalizer 对应的控制器已不存在

**解决**：
```bash
# 🟡 查看阻塞的 Finalizer
kubectl get ns my-ns -o jsonpath='{.spec.finalizers}'
# 🟡 强制移除 Finalizer
kubectl get ns my-ns -o json | jq '.spec.finalizers=[]' | kubectl replace --raw "/api/v1/namespaces/my-ns/finalize" -f -
```

### 案例 4：containerd 镜像 GC 导致磁盘占满

**现象**：节点 DiskPressure，Pod 被驱逐

**根因**：containerd 镜像垃圾回收阈值配置不当，大量未使用镜像堆积

**解决**：
```toml
# /etc/containerd/config.toml
[plugins."io.containerd.gc.v1.scheduler"]
  pause_threshold = 0.02
  deletion_threshold = 0
  mutation_threshold = 100
  schedule_delay = "0s"
  startup_delay = "100ms"
# kubelet 镜像 GC 配置
# imageGCHighThresholdPercent: 85
# imageGCLowThresholdPercent: 80
```

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| API Server 无响应 | etcd 不可用/过载 | etcd 健康检查、磁盘 IO |
| 节点 NotReady | kubelet 崩溃/证书过期 | `journalctl -u kubelet`、证书检查 |
| Pod 一直 Pending | 资源不足/调度约束 | `kubectl describe pod`、事件 |
| CrashLoopBackOff | 应用崩溃/探针失败 | 日志、探针配置 |
| etcd 集群不健康 | 成员失联/磁盘慢 | `etcdctl endpoint health` |
| 控制器不工作 | Leader 选举失败 | controller-manager 日志 |

## 命令速查

```bash
# 🟢 集群状态总览
kubectl cluster-info
kubectl get componentstatuses
kubectl get nodes -o wide

# 🟢 API Server 健康
curl -k https://<apiserver>:6443/healthz
curl -k https://<apiserver>:6443/livez?verbose
curl -k https://<apiserver>:6443/readyz?verbose

# 🟢 查看 API 资源
kubectl api-resources --sort-by=name
kubectl api-versions

# 🟢 查看对象详情
kubectl get pod myapp -o yaml
kubectl describe pod myapp
kubectl get events --sort-by=.metadata.creationTimestamp

# 🟡 强制删除 Pod
kubectl delete pod myapp --grace-period=0 --force

# 🟢 查看节点资源
kubectl describe node worker-1
kubectl top nodes

# 🟢 查看控制平面组件状态
kubectl get pods -n kube-system -l component=kube-apiserver
kubectl get pods -n kube-system -l component=kube-scheduler
kubectl get pods -n kube-system -l component=kube-controller-manager

# 🟢 查看 etcd 集群状态（通过 Pod）
kubectl exec -n kube-system etcd-master-1 -- etcdctl endpoint status --write-out=table

# 🟢 查看节点条件
kubectl get nodes -o custom-columns='NAME:.metadata.name,STATUS:.status.conditions[-1].type,REASON:.status.conditions[-1].reason'

# 🟢 查看集群事件（按时间排序）
kubectl get events -A --sort-by=.metadata.creationTimestamp | tail -20
```

## FAQ

**Q: 为什么 Kubernetes 移除了 dockershim？**
A: Docker 不是 CRI 兼容运行时，需要 dockershim 转换层。移除后直接使用 containerd/CRI-O，减少一层抽象，提升性能和稳定性。开发时仍可用 Docker 构建镜像。

**Q: etcd 为什么要求 SSD？**
A: etcd 使用 WAL（Write-Ahead Log），每次写操作都需要 fsync 到磁盘。HDD 的 fsync 延迟 ~10ms，SSD ~0.1ms。高延迟会导致 Raft 心跳超时、Leader 切换、集群不可用。

**Q: 控制器模式为什么用 Level-triggered 而非 Edge-triggered？**
A: Level-triggered 关注“当前状态是否符合期望”，即使丢失事件也能通过下次 Reconcile 修复；Edge-triggered 依赖事件不丢失，更脆弱。这保证了 K8s 的自愈能力。

**Q: Namespace 删除为什么可能卡住？**
A: Namespace 有 Finalizer 机制，删除前会等待所有子资源清理完成。如果某个 CRD 的控制器不存在，Finalizer 无法移除，Namespace 就会卡在 Terminating。

**Q: 容器运行时如何选择？**
A: 生产环境推荐 containerd（CNCF 毕业、生态成熟、轻量）；需要强隔离选 Kata Containers（VM 级隔离）；资源受限环境选 CRI-O（Red Hat 维护、无冗余）。Docker 仅用于开发构建，不用于生产运行时。

**Q: kube-apiserver 为什么不能直接连 etcd 集群外？**
A: etcd 存储的是集群全量状态，直接暴露会带来巨大安全风险。API Server 提供认证、授权、准入控制、API 版本转换、Watch 缓存等能力，是唯一合法的访问入口。

**Q: 为什么 K8s 使用声明式 API 而非命令式？**
A: 声明式（“我要什么”）而非命令式（“怎么做”）的优势：1) 幂等性，重复提交无副作用；2) 自愈，控制器持续协调；3) 可审计，状态可追溯；4) 解耦，用户无需关心实现细节。

## 版本兼容矩阵

| 组件 | 当前版本 | 关键变更 |
|------|---------|----------|
| Kubernetes | 1.31 | InPlace Pod Resize GA |
| etcd | 3.5 | 性能优化、Watch 改进 |
| containerd | 2.0 | CRI v1 默认、Wasm 支持 |
| CRI-O | 1.31 | 与 K8s 同步发布 |
| runc | 1.2 | cgroup v2 完善 |
| Kata | 3.10 | 多 Hypervisor 支持 |

## 缩略语表

| 缩略语 | 全称 | 说明 |
|--------|------|------|
| CRI | Container Runtime Interface | 容器运行时接口 |
| OCI | Open Container Initiative | 开放容器标准 |
| API | Application Programming Interface | 应用编程接口 |
| RBAC | Role-Based Access Control | 基于角色的访问控制 |
| GC | Garbage Collection | 垃圾回收 |
| PLEG | Pod Lifecycle Event Generator | Pod 生命周期事件生成器 |
| WAL | Write-Ahead Log | 预写日志 |
| mTLS | Mutual TLS | 双向 TLS |

## 学习路径

```
基础: Pod/Namespace/Node → kubectl → 控制器模式
进阶: API Server 流程 → etcd 运维 → kubelet 机制
高级: CRI/OCI → 调度器扩展 → 自定义控制器
专家: 内核机制(cgroup/eBPF) → 运行时开发 → API 扩展
```

## 检查清单

### 集群基础就绪检查

- [ ] etcd 集群健康（3/5 节点）且磁盘为 SSD
- [ ] API Server 高可用（≥ 2 副本 + LB）
- [ ] kubelet 证书自动轮换已启用
- [ ] 节点监控已部署（Node Exporter）
- [ ] 控制平面日志已采集
- [ ] etcd 定期备份已配置（每日自动 + 升级前手动）
- [ ] 集群版本偏差 ≤ 2 个小版本
- [ ] Cgroup v2 已启用（K8s 1.25+）
- [ ] 节点资源预留已配置（system-reserved/kube-reserved）
- [ ] Pod 安全准入已启用（restricted 级别）
- [ ] 集群审计日志已开启（Audit Policy）
- [ ] 控制平面组件资源限制已设置

## 参考链接

- https://kubernetes.io/docs/concepts/overview/components/
- https://kubernetes.io/docs/concepts/architecture/
- https://etcd.io/docs/
- https://containerd.io/
- https://github.com/opencontainers/runtime-spec
- https://kubernetes.io/docs/reference/command-line-tools-reference/
- https://kubernetes.io/docs/concepts/overview/kubernetes-api/
- https://kubernetes.io/docs/concepts/workloads/pods/
- https://github.com/cncf/toc/blob/main/DEFINITION.md

## Related

- [[17-系统基础/06-知识字典/fundamentals/kube-scheduler.md|kube-scheduler 调度]]
- [[17-系统基础/06-知识字典/fundamentals/kube-proxy.md|kube-proxy 网络]]
- [[17-系统基础/06-知识字典/security/rbac.md|RBAC 权限]]
- [[17-系统基础/06-知识字典/platform-engineering/operator-pattern.md|Operator 模式]]

<!-- risk-assessed -->
