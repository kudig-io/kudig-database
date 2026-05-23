---
title: Kubelet 驱逐阈值量化完整文档
description: '**文档类型**: 运维参考手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05'
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- kubelet
- prometheus
- rag
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Kubelet 驱逐阈值量化完整文档 是什么
- 如何 Kubelet 驱逐阈值量化完整文档
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- Kubelet
- 驱逐阈值量化完整文档
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
- type: domain
  path: ../domain-05-security-compliance/
  label: '相关知识域: domain-05-security-compliance'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
created: "2026-05-23"
---

# [[kubelet|Kubelet]] 驱逐阈值量化完整文档

> **文档类型**: 运维参考手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 判断"节点是否应该驱逐 Pod"、"为什么 Pod 被驱逐"、"如何配置驱逐阈值"

---

<!-- chunk: 1. Eviction Signal 完整列表 -->
## 1. Eviction Signal 完整列表

Kubelet 会持续监控以下信号，当信号达到阈值时会触发 Pod 驱逐。

### 1.1 可用信号列表

| Signal 名称 | 描述 | 默认阈值 | 单位 | 说明 |
|------------|------|---------|------|------|
| `memory.available` | 节点内存可用量 | `< 100Mi` | 绝对值 | kubelet 预留后剩余 |
| `nodefs.available` | 节点根文件系统可用空间 | `< 10%` | 百分比 | /var/lib/kubelet 等 |
| `nodefs.inodesFree` | 节点根文件系统 inodes 可用量 | `< 5%` | 百分比 | inode 耗尽会阻止创建 |
| `imagefs.available` | 容器镜像存储文件系统可用空间 | `< 15%` | 百分比 | 镜像存储专用卷 |
| `imagefs.inodesFree` | 镜像存储 inodes 可用量 | `< 5%` | 百分比 | |
| `pid.available` | 可用 Process ID 数量 | `< 1000` | 绝对值 | K8s 1.28+ |
| `imagefs.available` | **K8s 1.33 新增** | `< 10%` | 百分比 | 阈值可能调整 |

### 1.2 信号计算方式

```bash
# memory.available 计算逻辑
memory.available = node.capacity.memory - node.allocatable.memory - 系统基础预留 - kubelet 预留 - 现有 Pod 使用量

# 计算示例（64GB 节点）
# node.capacity.memory = 64Gi
# node.allocatable.memory = 60Gi（预留 4Gi 给系统）
# kubelet system-reserved = 1Gi
# kubelet kube-reserved = 500Mi
# 当前 Pod 使用 = 50Gi
# memory.available = 64 - 60 - 1.5 - 50 = 2.5Gi = 2.5 * 1024 = 2560Mi
# 触发条件: memory.available < 100Mi (默认)
```

```bash
# nodefs.available 计算逻辑
nodefs.available = (nodefs.capacity - nodefs.used) / nodefs.capacity
# 触发条件: nodefs.available < 10%
```

---

<!-- chunk: 2. Hard Eviction vs Soft Eviction -->
## 2. Hard Eviction vs Soft Eviction

### 2.1 Hard Eviction（硬驱逐）

**定义**：当信号达到阈值时，无宽限期，立即驱逐 Pod。

**配置方式**：
```yaml
# kubelet 配置（/var/lib/kubelet/config.yaml）
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
  nodefs.inodesFree: "5%"
  imagefs.inodesFree: "5%"
```

**特点**：
- 无 grace period
- 可能导致业务中断
- 应设置合理的阈值避免误触发

### 2.2 Soft Eviction（软驱逐）

**定义**：当信号超过阈值但未达到硬驱逐条件时，给 Pod grace period 后才驱逐。

**配置方式**：
```yaml
evictionSoft:
  memory.available: "500Mi"
  nodefs.available: "15%"
  imagefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "30s"
  nodefs.available: "30s"
  imagefs.available: "30s"
evictionPressureTransitionPeriod: "30s"  # 退出 eviction pressure 的等待时间
```

**特点**：
- 允许 Pod 有grace period 处理（如优雅关闭连接）
- 仅在 evictionPressureTransitionPeriod 内持续超阈值才触发
- 适合生产环境，避免意外中断

### 2.3 两者对比

| 维度 | Hard Eviction | Soft Eviction |
|------|-------------|---------------|
| Grace Period | 无（立即驱逐） | 有（可配置，如 30s/1m/5m） |
| 触发条件 | 信号达到硬阈值 | 信号超过软阈值且持续超过 grace period |
| 适用场景 | 测试环境 | 生产环境 |
| 风险 | 可能导致业务中断 | 较低风险 |

---

<!-- chunk: 3. 驱逐压力（Eviction Pressure）与恢复 -->
## 3. 驱逐压力（Eviction Pressure）与恢复

### 3.1 Eviction Pressure 判断

当 kubelet 判断节点资源不足时，会将节点设置为 `MemoryPressure` / `DiskPressure` condition：

```bash
# 查看节点 condition
kubectl describe node <node-name> | grep -A10 "Conditions"

# 输出示例
Type             Status
MemoryPressure   True    # 内存压力大
DiskPressure     False   # 磁盘正常
PIDPressure      False   # PID 正常
Ready            True    # 节点仍 Ready（但可能不再调度新 Pod）
```

### 3.2 驱逐压力状态转换

```
正常状态 → 压力上升 → Soft Eviction 触发 → 等待 grace period
正常状态 → 压力继续上升 → Hard Eviction 触发 → 立即驱逐
压力降低 → 退出 eviction pressure → 节点恢复正常调度
```

### 3.3 配置退出延迟

```yaml
evictionPressureTransitionPeriod: "30s"  # 压力降低后需持续 30s 才退出 eviction 状态
```

---

<!-- chunk: 4. Kubelet 资源预留与 Eviction 关系 -->
## 4. Kubelet 资源预留与 Eviction 关系

### 4.1 预留配置

```yaml
# /var/lib/kubelet/config.yaml
systemReserved:
  cpu: "100m"
  memory: "1Gi"
  ephemeral-storage: "1Gi"
kubeReserved:
  cpu: "100m"
  memory: "500Mi"
  ephemeral-storage: "1Gi"
evictionHard:
  memory.available: "100Mi"  # 基于 allocatable 计算后的值
```

### 4.2 预留与 Eviction 阈值关系

```
节点总内存 = 64Gi
系统预留 = 4Gi
kubelet 预留 = 1.5Gi (system-reserved + kube-reserved)
allocatable = 64 - 4 - 1.5 = 58.5Gi

当所有 Pod 使用 58Gi 时：
memory.available = 58.5Gi - 58Gi = 0.5Gi = 512Mi

如果 evictionHard.memory.available = 100Mi：
512Mi > 100Mi → 不会触发 hard eviction

如果 Pod 继续使用到 58.4Gi：
memory.available = 58.5 - 58.4 = 0.1Gi = 102Mi
102Mi > 100Mi → 接近触发，但不会立即触发

如果 Pod 达到 58.45Gi：
memory.available = 58.5 - 58.45 = 0.05Gi = 51Mi
51Mi < 100Mi → 触发 hard eviction
```

### 4.3 预留配置建议

| 节点规格 | system-reserved | kube-reserved | 建议 evictionHard.memory.available |
|---------|-----------------|--------------|----------------------------------|
| 4C8G | 500m / 500Mi | 250m / 250Mi | 100Mi |
| 8C16G | 1C / 1Gi | 500m / 500Mi | 200Mi |
| 16C32G | 1C / 2Gi | 1C / 1Gi | 500Mi |
| 32C64G | 2C / 2Gi | 1C / 2Gi | 1Gi |
| 64C128G | 2C / 4Gi | 2C / 4Gi | 2Gi |

---

<!-- chunk: 5. ImageFs 与 NodeFs 的区分 -->
## 5. ImageFs 与 NodeFs 的区分

### 5.1 何时有两个文件系统

```
情况 1: 容器镜像和 kubelet 使用同一磁盘
  → 只有 nodefs.available（无独立的 imagefs）

情况 2: 容器镜像使用独立磁盘（/var/lib/container）
  → nodefs.available = 根文件系统（/）
  → imagefs.available = 镜像存储专用磁盘
```

### 5.2 常见场景

| 场景 | imagefs | nodefs | 说明 |
|------|---------|--------|------|
| 单磁盘节点 | 无 | / (root) | 镜像和 kubelet 共用根分区 |
| 独立镜像盘 | /var/lib/container | / (root) | 云厂商常见（如 AWS 的 100GB root + EBS 数据盘） |
| 本地临时存储分离 | /mnt/disk1 | / (root) | 高性能本地 NVMe SSD 场景 |

### 5.3 检查节点文件系统配置

```bash
# 在节点上查看挂载点
df -h
# 输出示例：
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/nvme0n1p1  100G  50G   50G  50% /
# /dev/nvme1n1    500G  10G  490G   2% /var/lib/container  # 独立镜像盘

# 查看 kubelet 使用的 filesystem
ls /var/lib/kubelet/
```

---

<!-- chunk: 6. Pod 驱逐顺序（Eviction Strategy） -->
## 6. Pod 驱逐顺序（Eviction Strategy）

### 6.1 驱逐优先级

Kubelet 按以下顺序驱逐 Pod（先驱逐低优先级）：

```
BestEffort Pod（最低优先级）
    ↓
Burstable Pod（中等优先级）
    ↓
Guaranteed Pod（最高优先级，同 QoS 等级按 resource usage 排序）
```

### 6.2 QoS 等级判断

| QoS 等级 | 条件 | 优先级 |
|---------|------|--------|
| Guaranteed | 所有容器的 CPU/Memory limits 和 requests 相等且设置了 | 最高（最后驱逐） |
| Burstable | 不满足 Guaranteed 但设置了 requests | 中等 |
| BestEffort | 没有任何容器设置了 requests 或 limits | 最低（最先驱逐） |

### 6.3 同 QoS 等级内排序

相同 QoS 等级的 Pod 按以下顺序驱逐：
1. 实际资源使用量（使用越多越先驱逐）
2. 创建时间（越早创建越先驱逐）
3. 优先级（priority 值越低越先驱逐）

---

<!-- chunk: 7. Grace Period 配置 -->
## 7. Grace Period 配置

### 7.1 每个 signal 的独立 grace period

```yaml
evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "30s"
  imagefs.available: "30s"
  nodefs.inodesFree: "1m"
  imagefs.inodesFree: "1m"
  pid.available: "1m"
```

### 7.2 全局 evictionPressureTransitionPeriod

```yaml
evictionPressureTransitionPeriod: "30s"
# 含义：节点从 "memory pressure" 状态恢复到正常需要持续 30s
```

---

<!-- chunk: 8. Pod 优雅终止与 Eviction 的关系 -->
## 8. Pod 优雅终止与 Eviction 的关系

### 8.1 驱逐时的优雅终止

当 Pod 被 eviction 驱逐时：
1. kubelet 发送 SIGTERM 到容器
2. 等待 `terminationGracePeriodSeconds`（默认 30s）
3. 如容器未在 grace period 内终止，发送 SIGKILL

### 8.2 如何在 eviction 时增加 graceful 时间

```yaml
# Pod spec
apiVersion: v1
kind: Pod
spec:
  terminationGracePeriodSeconds: 60  # 增加优雅关闭时间
  containers:
  - name: nginx
    image: nginx
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 30 && /usr/sbin/nginx -s quit"]
    # 给 nginx 额外时间处理现有连接
```

---

<!-- chunk: 9. 监控与告警配置 -->
## 9. 监控与告警配置

### 9.1 推荐的 [[Prometheus|Prometheus]] 告警规则

```yaml
groups:
- name: eviction-alerts
  rules:
  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure", status="true"} == 1
    for: 1m
    labels:
      severity: warning
    annotations:
      description: "Node {{ $labels.node }} is under memory pressure"

  - alert: NodeDiskPressure
    expr: kube_node_status_condition{condition="DiskPressure", status="true"} == 1
    for: 1m
    labels:
      severity: warning
    annotations:
      description: "Node {{ $labels.node }} is under disk pressure"

  - alert: NodeMemoryPressureCritical
    expr: (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) < 0.05
    for: 30s
    labels:
      severity: critical
    annotations:
      description: "Node {{ $labels.node }} memory available < 5%"
```

### 9.2 kubectl 查看节点资源

```bash
# 查看节点 allocatable 和容量
kubectl describe node <node-name> | grep -A10 "Allocatable"

# 查看节点资源使用（需 metrics-server）
kubectl top node <node-name>

# 查看各 Pod 资源使用
kubectl top pods -A --sort-by=memory | head -20
```

---

<!-- chunk: 10. 配置示例 -->
## 10. 配置示例

### 10.1 开发/测试环境（宽松阈值）

```yaml
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "5%"
  imagefs.available: "5%"
evictionSoft: {}
evictionSoftGracePeriod: {}
```

### 10.2 生产环境（推荐配置）

```yaml
evictionHard:
  memory.available: "500Mi"  # 留更多余量
  nodefs.available: "10%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "15%"
  imagefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "30s"
  imagefs.available: "30s"
evictionPressureTransitionPeriod: "30s"
```

### 10.3 高性能计算节点（宽松内存）

```yaml
evictionHard:
  memory.available: "2Gi"  # 高内存节点预留更多
  nodefs.available: "10%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "4Gi"
evictionSoftGracePeriod:
  memory.available: "2m"
```

---

<!-- chunk: 11. 故障排查 -->
## 11. 故障排查

### 11.1 如何判断 Pod 被 Eviction 的原因

```bash
# 1. 查看 Pod 状态
kubectl get pod <pod-name> -o yaml | grep -i reason
# 找 "Evicted" 或 "OutOfDisk"

# 2. 查看 kubelet 日志（节点上）
journalctl -u kubelet | grep -i "evict"

# 3. 查看节点 condition
kubectl describe node <node-name> | grep -i "Pressure"

# 4. 查看 node 级 event
kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp' | tail -20
```

### 11.2 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Pod 被 Evicted 但节点内存还有 20% | systemReserved/kubeReserved 配置过低 | 增加预留，或提高 evictionHard 阈值 |
| Soft eviction 未触发 | evictionSoftGracePeriod 设置过长 | 缩短 grace period |
| eviction pressure 无法恢复 | 根本原因未解决（内存泄漏/磁盘满） | 解决根本问题 |
| Pod 被 Evicted 但 QoS 是 Guaranteed | 系统资源严重不足，Guaranteed 也无法保护 | 扩容节点或减少 Pod 数量 |

---

<!-- chunk: 附录：完整配置字段参考 -->
## 附录：完整配置字段参考

```yaml
# /var/lib/kubelet/config.yaml 完整 eviction 配置
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
  nodefs.inodesFree: "5%"
  imagefs.inodesFree: "5%"
  # pid.available: "1000"  # K8s 1.28+

evictionSoft:
  memory.available: "500Mi"
  nodefs.available: "15%"
  imagefs.available: "20%"

evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "30s"
  imagefs.available: "30s"

evictionPressureTransitionPeriod: "30s"

# eviction 临终关怀（grace period 给被驱逐的 Pod）
evictionTerminatingGracePeriod:
  required: false  # K8s 1.27+ 移除，改用 terminationGracePeriodSeconds
```

---

```yaml
---
id: KUBELET-EVICTION-001
domain: control-plane
type: operation-guide
tags: [kubelet, eviction, node-pressure, resource-management, k8s-1.28-1.33, agent-corpus]
intent_queries:
  - "Kubelet 驱逐阈值怎么配置"
  - "memory.available 100Mi 是什么意思"
  - "Pod 被 Evicted 怎么排查"
  - "hard eviction 和 soft eviction 的区别"
  - "QoS 等级和驱逐优先级的对应关系"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - domain-01-cluster-fundamentals/15-kubelet-deep-dive.md
  - domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md
  - domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals KUDIG Database — Global MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## See Also

- 32-kubeadm-cluster-lifecycle
- 32-kubeadm-upgrade-complete-guide
- final-completion-check
- quality-report
