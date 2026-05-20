---
title: 协调领导者选举（Coordinated Leader Election）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- scheduler
- controller-manager
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 协调领导者选举（Coordinated Leader Election） 是什么
- 如何 协调领导者选举（Coordinated Leader Election）
trigger_keywords:
- 协调领导者选举
- Coordinated
- Leader
- Election
- dictionary
title_en: Coordinated Leader Election
---


# 协调领导者选举（Coordinated Leader Election）

## 概述

FEATURE STATE: `Kubernetes v1.33 [beta]`（默认禁用）

Kubernetes 1.35 引入了一项 beta 特性，允许控制平面组件通过**协调领导者选举（Coordinated Leader Election）**确定性地选择领导者。该特性主要用于满足 Kubernetes 集群升级期间的版本倾斜约束。当前内置的选择策略是 `OldestEmulationVersion`，优先选择模拟版本最低的候选者，其次是二进制版本，最后是创建时间戳。

## 核心概念/原理

- **Lease API**：Kubernetes 使用 Lease API 作为轻量级分布式锁，在高可用集群中的同一控制平面组件（如 `kube-controller-manager`、`kube-scheduler`）多个实例之间执行领导者选举。
- **LeaseCandidate API**：控制平面组件通过创建 LeaseCandidate 对象注册为候选者，携带候选者的身份、二进制版本和模拟版本等元数据。
- **乐观并发控制**：当 Lease 不存在或过期时，多个候选者尝试通过 `resourceVersion` 更新 Lease，仅有一个更新成功并成为领导者。
- **协调选举**：相比传统的抢锁式选举，协调选举允许基于候选者的元数据（如版本信息）有策略地选择领导者。

## 关键机制或特性

### 启用协调领导者选举

需要同时满足以下条件：

1. 启用 `CoordinatedLeaderElection` 特性门控。
2. 启用 `coordination.k8s.io/v1beta1` API 组。

通过以下 kube-apiserver 启动参数启用：

```bash
--feature-gates="CoordinatedLeaderElection=true"
--runtime-config="coordination.k8s.io/v1beta1=true"
```

### 组件自动使用

在 Kubernetes 1.35 中，当特性门控和 API 组均启用时，以下两个控制平面组件会自动使用协调领导者选举：

- `kube-controller-manager`
- `kube-scheduler`

### Lease API 字段

Lease 对象包含以下关键字段：

- `holderIdentity`：当前领导者的身份（如 Pod 名称或基于主机名的字符串）。
- `acquireTime`：获得领导权的时间戳。
- `renewTime`：领导者最近一次续租的时间戳。
- `leaseDurationSeconds`：租约有效期（候选者应等待 `renewTime + leaseDurationSeconds` 加上少量宽限期后才尝试获取过期租约）。
- `leaseTransitions`：领导权变更次数的计数器。

### 领导者选举流程

1. 所有运行中的组件实例监视或定期读取 Lease 对象，确认当前领导者。
2. 当 Lease 不存在或已过期（当前时间 > `renewTime + leaseDurationSeconds`）时，候选者尝试更新 Lease。
3. 通过乐观并发控制，只有一个更新成功，该实例成为领导者。
4. 领导者定期更新 `renewTime`（通常每 `leaseDurationSeconds / 2` 续租一次）。
5. 如果领导者崩溃、不可达或停止续租，租约过期，其他健康实例检测并发起新一轮选举。

### 协调选举策略

当前唯一内置策略：`OldestEmulationVersion`

选择优先级：
1. 模拟版本最低（emulation version 最小）
2. 二进制版本最低
3. 创建时间戳最早

这有助于在升级过程中，让模拟旧版本的实例优先担任领导者，保持与旧版本行为的兼容性。

## 使用场景

- **集群升级期间的版本倾斜管理**：在滚动升级控制平面时，确保领导者由兼容旧版本的实例担任，降低升级风险。
- **高可用控制平面**：在多个控制平面实例之间稳定、可预测地选举领导者。
- **确定性领导者选择**：基于版本策略而非纯随机抢锁，提供更可控的升级行为。

## 最佳实践/注意事项

- 在启用协调领导者选举前，确保 kube-apiserver 已启用 `CoordinatedLeaderElection` 特性门控和 `coordination.k8s.io/v1beta1` API 组。
- 该特性在 Kubernetes 1.35 中仍为 beta 且默认禁用，生产环境中启用前应在 staging 环境充分测试。
- 协调领导者选举不改变 Lease 机制的核心行为（续租、过期检测、抢锁），只是增加了基于元数据的选举策略。
- 当集群中存在混合版本实例时，协调选举有助于平滑过渡，但仍需遵循 Kubernetes 的版本倾斜策略。
- 监控 Lease 和 LeaseCandidate 对象的状态，及时发现选举异常或领导者频繁切换的问题。

## 故障排查

| 症状 | 可能原因 | 排查命令/方法 |
|------|---------|-------------|
| 组件未使用协调选举 | 特性门控未启用或 API 组未注册 | 检查 apiserver 启动参数 `--feature-gates` 和 `--runtime-config` |
| LeaseCandidate 对象未创建 | 组件版本不支持或未启用特性 | `kubectl get leasecandidates -n kube-system` |
| 领导者频繁切换 | 网络不稳定或续租间隔过短 | `kubectl describe lease <name> -n kube-system` 查看 leaseTransitions |
| 升级后新版本实例成为领导者 | 策略期望旧版本优先但未生效 | 检查 LeaseCandidate 的 emulationVersion 字段 |
| Lease 对象过期未恢复 | 所有候选者不可用 | `kubectl get lease -n kube-system` 检查 renewTime |
| controller-manager 无法获取领导权 | Lease 被其他实例持有且未过期 | `kubectl get lease kube-controller-manager -n kube-system -o yaml` |
| scheduler 双主运行 | Lease 机制失效，多实例同时活跃 | 检查所有 scheduler Pod 日志中的 "became leader" 消息 |

## 生产检查清单

- [ ] 协调领导者选举仅在 staging 环境充分测试后再启用到生产
- [ ] `CoordinatedLeaderElection` 特性门控和 `coordination.k8s.io/v1beta1` API 同时启用
- [ ] 监控 Lease 对象的 `leaseTransitions` 字段，频繁变更需告警
- [ ] 监控 Lease 的 `renewTime`，超过 `leaseDurationSeconds` 未续租需告警
- [ ] 混合版本升级时确认 `OldestEmulationVersion` 策略正确选择低版本实例
- [ ] 控制平面组件的 `--leader-elect-lease-duration` 和 `--leader-elect-renew-deadline` 参数合理
- [ ] etcd 网络延迟低于 Lease 续租间隔的 1/3
- [ ] 升级完成后验证所有 LeaseCandidate 版本信息正确

## 命令快速参考

```bash
# 查看 kube-system 中的 Lease 对象
kubectl get lease -n kube-system

# 查看 controller-manager 领导者信息
kubectl get lease kube-controller-manager -n kube-system -o yaml

# 查看 scheduler 领导者信息
kubectl get lease kube-scheduler -n kube-system -o yaml

# 查看 LeaseCandidate 对象（协调选举）
kubectl get leasecandidates -n kube-system

# 查看 LeaseCandidate 详情（版本信息）
kubectl describe leasecandidate -n kube-system

# 查看当前领导者身份
kubectl get lease kube-controller-manager -n kube-system -o jsonpath='{.spec.holderIdentity}'

# 查看领导权变更次数
kubectl get lease kube-controller-manager -n kube-system -o jsonpath='{.spec.leaseTransitions}'

# 查看最近续租时间
kubectl get lease kube-controller-manager -n kube-system -o jsonpath='{.spec.renewTime}'

# 检查控制平面组件日志中的选举信息
kubectl -n kube-system logs -l component=kube-controller-manager | grep -i "leader"
kubectl -n kube-system logs -l component=kube-scheduler | grep -i "leader"
```

## 交叉引用

- [api-priority-and-fairness.md](./api-priority-and-fairness.md) — leader election 请求的 APF 优先级保障
- [compatibility-version-for-control-plane.md](./compatibility-version-for-control-plane.md) — 控制平面版本兼容与模拟版本
- [operator-pattern.md](./operator-pattern.md) — Operator 中的 leader election 实现
- [../scheduling/kubernetes-scheduler.md](../scheduling/kubernetes-scheduler.md) — kube-scheduler 的 HA 与选举

## 参考链接

- [Coordinated Leader Election - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/coordinated-leader-election/)
