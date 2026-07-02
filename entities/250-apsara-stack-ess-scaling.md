---
title: 专有云 (Apsara Stack) - ESS 弹性伸缩
description: 'description: ''- [专有云 ESS 架构差异](#专有云-ess-架构差异)'''
summary: 'description: ''- [专有云 ESS 架构差异](#专有云-ess-架构差异)'''
category: general
tags:
- cloud
- multi-cloud
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 专有云 (Apsara Stack) - ESS 弹性伸缩 是什么
- 如何 专有云 (Apsara Stack) - ESS 弹性伸缩
- Kubernetes 12 cloud providers 最佳实践
trigger_keywords:
- 专有云
- Apsara
- Stack
- ESS
- 弹性伸缩
- cloud
- providers
prerequisites:
- kubectl-basics
- troubleshooting-methodology
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 专有云 (Apsara Stack) - ESS 弹性伸缩
description: '- [专有云 ESS 架构差异](#专有云-ess-架构差异)'
category: cloud-provider
tags:
- k8s
- cloud
- eks
- gke
- aks
- ack
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 云架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 专有云 (Apsara Stack) - ESS 弹性伸缩 是什么
- 如何 专有云 (Apsara Stack) - ESS 弹性伸缩
- [[Kubernetes|Kubernetes]] 17 cloud provider 最佳实践
trigger_keywords:
- 专有云
- Apsara
- Stack
- ESS
- 弹性伸缩
- cloud
- provider
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# 专有云 (Apsara Stack) - ESS 弹性伸缩

> **环境**: Apsara Stack 企业版/精简版 | **最后更新**: 2026-01

---

## 目录

- [专有云 ESS 架构差异](#专有云-ess-架构差异)
- [伸缩组 (Scaling Group) 配置](#伸缩组-scaling-group-配置)
- [伸缩触发策略与健康检查](#伸缩触发策略与健康检查)
- [与 ACK 节点自动伸缩集成](#与-ack-节点自动伸缩集成)
- [常见场景与故障排查](#常见场景与故障排查)

---

## 专有云 ESS 架构差异

在专有云环境下，ESS 的底层依赖与公有云存在显著差异，主要体现在资源交付的确定性和底层隔离性上。

| 维度 | 专有云 (Apsara Stack) | 公共云 |
|:---|:---|:---|
| **资源池** | 私有物理机集群 (Compute Container) | 共享资源池 |
| **API 接入** | POP 网关 (内网) | 公网/内网 Open API |
| **伸缩限制** | 受限于物理资源配额 (Quota) | 几乎无限扩展 |
| **付费模式** | 资源预存/内部计费 | 按量/包年包月 |

---

## 伸缩组 (Scaling Group) 配置

### 核心参数规划

| 参数 | 专有云配置建议 | 说明 |
|:---|:---|:---|
| **移除策略** | `OldestInstance` | 优先回收存活时间最长的旧实例 |
| **后端服务器组** | 挂载 SLB / NLB | 自动将扩容实例加入 LB 服务 |
| **期望记录数** | 匹配业务基准 | 伸缩组将维持实例数在此数值 |
| **保护机制** | 启用实例保护 | 防止特定的核心实例被意外缩容 |

---

## 伸缩触发策略与健康检查

### 触发模式对比

- **定时触发 (Scheduled Task)**: 适用于业务可预期的波动 (如早晚高峰)。
- **报警触发 (Alarm Task)**: 基于 CPU/内存等性能指标。在专有云中，报警由 **CloudMonitor** 监控组件推送到 ESS。
- **健康检查 (Health Check)**: 伸缩组会自动替换状态为非 `Running` 的 ECS 实例。

### 监控指标速查表

| 指标名称 | 单位 | 建议阈值 (扩容) | 建议阈值 (缩容) |
|:---|:---:|:---:|:---:|
| `CpuUtilization` | % | > 75% | < 30% |
| `MemoryUtilization` | % | > 80% | < 40% |
| `IntranetIn` | bits/s | 按业务带宽规划 | - |

---

## 与 ACK 节点自动伸缩集成

在专有云 ACK 环境中，伸缩由 **Cluster-Autoscaler** 调用专有云 POP 接口实现。

### 配置要点

1. **ServiceAccount 授权**: 确保 `cluster-autoscaler` 具有调用专有云 ESS 的权限。
2. **节点池映射**: 每个 ACK 节点池对应一个专有云 ESS 伸缩组。
3. **资源配额**: 必须确保专有云控制台 (ASOP) 中分配给该租户的 ECS 配额大于等于伸缩组的最大值。

---

## 常见场景与故障排查

### 场景 1: 扩容失败 (资源不足)

- **现象**: 伸缩活动显示 `InsufficientCapacity`。
- **排查**: 检查专有云物理机集群 (Compute Container) 是否有剩余可用资源。
- **解决**: 申请扩充租户配额或联系平台管理员增加物理节点。

### 场景 2: 实例心跳超时

- **现象**: 实例创建成功但无法加入伸缩组。
- **排查**: 检查镜像中的 `aliyun-[[Service|service]]` 守护进程是否启动，专有云网络连接是否正常。

---

## 相关文档

- [240-ack-ecs-compute.md](./240-ack-ecs-compute.md) - 公共云 ECS 计算参考
- [252-apsara-stack-pop-operations.md](./252-apsara-stack-pop-operations.md) - 专有云平台运维 (POP)

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|[[Go 生产环境速查卡|go]]]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/240-ack-ecs-compute.md|240-ack-ecs-compute]]
- [[entities/252-apsara-stack-pop-operations.md|252-apsara-stack-pop-operations]]
- [[entities/kubernetes.md|kubernetes]]

## See Also

- [[domain-12-cloud-providers/15-alicloud-apsara-ack/252-apsara-stack-pop-operations.md|252-apsara-stack-pop-operations]]
- [[domain-12-cloud-providers/15-alicloud-apsara-ack/alicloud-apsara-ack-overview.md|alicloud-apsara-ack-overview]]
- [[domain-12-cloud-providers/15-alicloud-apsara-ack/251-apsara-stack-sls-logging.md|251-apsara-stack-sls-logging]]
- [[domain-12-cloud-providers/15-alicloud-apsara-ack/252-apsara-stack-pop-operations.md|252-apsara-stack-pop-operations]]


<!-- risk-assessed -->
