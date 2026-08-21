---
title: Linux 节点安全
description: '# Linux 节点安全'
summary: '# Linux 节点安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux 节点安全 是什么
- 如何 Linux 节点安全
trigger_keywords:
- Linux
- 节点安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Linux 节点安全

## 概述

本页面描述了针对 Linux 操作系统的安全考虑和最佳实践。Linux 节点在 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 集群中承担着运行容器工作负载的重要角色，某些内核和系统配置会直接影响 Secret 等敏感数据的保护效果。

## 核心概念/原理

在 Linux 节点上，基于内存的卷（例如 Secret 卷挂载，或设置了 `medium: Memory` 的 `emptyDir`）是通过 `tmpfs` 文件系统实现的。`tmpfs` 将数据存储在内存中，而不是持久化磁盘上，从而在正常情况下提供更好的保密性。

然而，如果节点启用了 **swap（交换分区/文件）**，并且使用的 Linux 内核版本较旧（或使用了不受支持的 Kubernetes 配置），则这些内存支持卷中的数据可能被写入到持久化的 swap 存储中，从而导致敏感数据泄露风险。

## 关键机制或特性

- **tmpfs 与 swap 的交互**：
  - `tmpfs` 的内容默认驻留在内存中。
  - 在旧内核或特定配置下，当系统内存压力较大时，`tmpfs` 页面可能被交换（swap）到磁盘。
- **noswap 选项**：
  - Linux 内核从 **6.3** 版本开始正式支持 `noswap` 挂载选项。
  - 启用 `noswap` 后，可以阻止 `tmpfs` 的内容被交换到磁盘，从而保护 Secret 等敏感内存数据。

## 使用场景

- 在启用了 swap 的 Linux 节点上运行 Kubernetes 工作负载。
- 对 Secret 数据保护有较高安全要求的集群环境。
- 需要评估和加固节点层面数据保密性的场景。

## 最佳实践/注意事项

- 如果 Linux 节点启用了 swap，建议将内核升级至 **6.3 或更高版本**（或通过 backport 获得 `noswap` 支持）。
- 阅读 Kubernetes 关于 swap 内存管理的官方文档，了解如何在集群中正确配置 swap 行为。
- 定期检查节点的 swap 配置和内核版本，确保敏感数据不会因 swap 机制而泄露到持久存储。
- 考虑在节点层面使用额外的加密措施（如全盘加密），以进一步降低数据泄露风险。

## 架构深度解析

### 节点安全与 swap 内存保护机制

```
┌──────────────────────────────────────────────────────────────┐
│  Linux 节点（kubelet 管理）                                   │
│   │                                                          │
│   ├─ swap 配置：/etc/fstab + systemd swap 单元               │
│   │  └─ kubelet --fail-swap-on / NodeSwap 特性门控           │
│   ├─ 内存层级：                                          │
│   │  ├─ 内存页（匿名页/文件页）                              │
│   │  ├─ tmpfs：RAM-backed 文件系统（Secret/emptyDir 挂载）   │
│   │  └─ swap：内存页换出到磁盘（zram/分区/文件）              │
│   └─ noswap 挂载选项（内核 ≥ 6.3）：                         │
│      └─ tmpfs 内容永不换出 → Secret 不落盘                   │
│                                                              │
│  风险链路：                                                   │
│  Secret 在 tmpfs → 内存压力 → 页面换出到 swap（磁盘）        │
│  → 攻击者读取磁盘（取证/冷启动/卸载磁盘）→ 敏感数据泄露      │
└──────────────────────────────────────────────────────────────┘
```

### 内核与 kubelet 关键路径

| 组件 | 位置/版本 | 关键机制 |
|---|---|---|
| noswap 挂载 | Linux 内核 ≥ 6.3（fs/namespace.c） | tmpfs 禁止换出 |
| NodeSwap 门控 | Kubernetes ≥ 1.22（特性门控） | swap 支持策略 |
| kubelet 行为 | pkg/kubelet/apis/config/ | fail-swap-on 与 swap 阈值 |
| tmpfs 实现 | mm/shmem.c | 匿名页换出机制 |
| Secret 存储 | kubelet pkg/volume/secret/ | tmpfs 挂载 Secret |

### 流程步骤

1. 节点启用 swap（分区/zram），kubelet 按 NodeSwap 门控策略处理（拒绝或允许）。
2. Pod 挂载 Secret/emptyDir（tmpfs 类型）时，数据驻留内存匿名页。
3. 内存压力下内核触发回收：匿名页被换出到 swap 设备（磁盘）。
4. 内核 ≥ 6.3 且挂载带 noswap 时，tmpfs 页面被排除在换出之外。
5. 敏感数据因此只存在于 RAM，断电/磁盘取证均无法恢复明文。

## 生产案例

### 案例 1：swap 换出导致 Secret 落盘泄露（2023 年安全审计发现）

| 时间 | 事件 |
|---|---|
| T+0 | 安全审计在节点磁盘上恢复出 Secret 明文片段 |
| T+1h | 定位为内存压力下 tmpfs（Secret 挂载）页面被换出到 swap 分区 |
| T+2h | 内核 < 6.3，无法使用 noswap；临时方案禁用 swap + 提升节点内存 |
| T+1w | 节点内核升级至 6.3+，tmpfs 挂载启用 noswap，根治问题 |

- **根因**：启用 swap 的节点 + 内核无 noswap 支持，Secret 页面可被换出落盘。
- **修复命令**（禁用 swap + 验证）：
```bash
# 🔴 立即关闭 swap（运行时）
swapoff -a && sed -i '/swap/d' /etc/fstab
# 🟢 验证 swap 已禁用且 tmpfs 挂载为 noswap
free -h && mount | grep tmpfs
```

### 案例 2：NodeSwap 门控误配置导致 kubelet 启动失败

- **现象**：节点启用 swap 后 kubelet 反复崩溃。
- **诊断**：kubelet 未配置 `--fail-swap-on=false` 且未启用 NodeSwap 特性门控，默认拒绝 swap 节点。
- **修复**：明确集群 swap 策略：要么统一禁用 swap（推荐），要么启用 NodeSwap 门控并按版本配置 `memorySwap.swapBehavior`（LimitedSwap/UnlimitedSwap）。

## 对比评测

| 维度 | swap 关闭（推荐） | NodeSwap（LimitedSwap） | noswap 挂载（内核 6.3+） |
|---|---|---|---|
| Secret 保护 | 强（不落盘） | 中（可配置限制） | 强（tmpfs 不换出） |
| 内存弹性 | 无（OOM 风险高） | 中（可换出匿名页） | 中（仅非 tmpfs 可换出） |
| 版本要求 | 无 | K8s ≥ 1.22 门控 | 内核 ≥ 6.3 |
| 运维复杂度 | 低 | 中 | 中 |
| 适用 | 严格安全场景 | 内存受限环境 | 需 swap + 保 Secret 场景 |

- **选型建议**：安全优先统一禁用 swap；内存受限且需 swap 用 NodeSwap LimitedSwap；必须 swap 且内核 6.3+ 用 noswap 保护 tmpfs。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| kubelet 启动失败 | swap 未禁用/门控未配 | `free -h`、`cat /proc/swaps`、kubelet 日志 |
| Secret 落盘 | tmpfs 被换出 | `grep Swap /proc/$(pgrep -f kubelet)/status`、`swapoff` |
| 内存不足 | swap 关闭后 OOM | 检查 node allocatable 与 Pod 内存请求 |
| noswap 不生效 | 内核版本不足 | `uname -r` 确认 ≥ 6.3 |
| 性能下降 | swap 换入换出频繁 | `vmstat 1` 观察 si/so 字段 |

## 生产部署清单

- [ ] 集群级 swap 策略明确（禁用或 LimitedSwap），写入节点基线镜像
- [ ] 敏感命名空间 Secret 挂载确认 tmpfs + noswap（内核 ≥ 6.3）
- [ ] 节点内存按工作负载峰值规划，避免 swap 关闭后的 OOM 风险
- [ ] 定期审计节点 swap 配置与内核版本（巡检脚本）
- [ ] 监控内存水位、swap 使用量、OOM 事件并告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 发现 Secret 落盘/swap 泄露风险 | 立即 swapoff + 清 swap 分区，评估影响面 |
| P1 | 节点内核升级至 6.3+ | 灰度批次升级，启用 noswap 挂载并回归验证 |
| P2 | 启用 NodeSwap 特性 | 先在测试集群验证 memorySwap 行为，再灰度 |

## 面试要点

> 以下 Q&A 覆盖 Linux 节点安全面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：为什么 Secret 存储在 tmpfs 仍可能泄露到磁盘？**
   A：tmpfs 是 RAM-backed 文件系统，但其中的内存页属于匿名页，在内核内存压力下会被回收机制换出（swap out）到 swap 设备（磁盘分区/文件/zram）。因此即使 Secret 挂载在 tmpfs，一旦页面换出，明文就落盘，攻击者可通过磁盘取证恢复。这就是 swap 对 Kubernetes 安全的根本威胁。

2. **Q：noswap 挂载选项如何解决这个问题？**
   A：Linux 内核 6.3+ 为 tmpfs 引入 noswap 挂载选项：挂载时设置后，该文件系统页面被标记为不可换出（类似 mlock 语义），内存压力下优先回收其他页面，Secret 等敏感数据永不落盘。Kubernetes 对敏感挂载（Secret 等）在支持的节点上自动使用该选项，但要求节点内核 ≥ 6.3。

3. **Q：生产集群对 swap 的正确策略是什么？**
   A：默认安全策略是禁用 swap（swapoff + 修改 fstab + kubelet 默认行为），换取确定性内存行为与 Secret 保护；内存受限环境可用 NodeSwap 特性门控 + LimitedSwap（限制换出额度）；必须保留 swap 的节点应升级内核 6.3+ 启用 noswap 保护敏感挂载，并监控换出量。核心原则：敏感数据永远不允许落盘。

## 运维要点

- 节点基线：swap 策略固化到节点镜像/启动脚本，新节点自动生效。
- 巡检：定期检查 /proc/swaps、fstab、内核版本、tmpfs 挂载选项。
- 监控：内存水位、swap 换入换出（si/so）、OOM 事件、Pod 驱逐。
- 应急：发现 Secret 落盘 → swapoff + 清理 swap + 轮换受影响 Secret。
- 升级联动：内核升级到 6.3+ 时同步启用 noswap，回归 Secret 挂载。

## 参考链接

- https://kubernetes.io/docs/concepts/security/linux-security/

## Related

- [[17-系统基础/06-知识字典/security/admission-controller.md|准入控制器]]
- [[17-系统基础/06-知识字典/security/application-security-checklist.md|应用安全清单]]
- [[17-系统基础/06-知识字典/security/athenz.md|Athenz 身份认证与授权]]


<!-- risk-assessed -->
