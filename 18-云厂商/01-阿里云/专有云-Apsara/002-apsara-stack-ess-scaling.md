---
title: 专有云（Apsara Stack）- ESS 弹性伸缩
description: 专有云 ESS 弹性伸缩架构差异、伸缩组配置、触发策略、与 ACK 节点自动伸缩集成及故障排查
summary: 专有云（Apsara Stack）ESS 弹性伸缩深度指南：架构差异、伸缩组配置、定时/报警/健康检查触发、与 ACK Cluster Autoscaler 集成、配额管理与故障排查。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- ess
- autoscaling
- cluster-autoscaler
- kubernetes
tier: core
sources:
- 阿里云专有云 ESS 运维手册
- ACK Cluster Autoscaler 集成指南
created: 2026-05-23
last_updated: 2026-07-23
relationships:
- target: '[[18-云厂商/01-阿里云/02-ACK集群运维.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/004-apsara-stack-pop-operations.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/01-apsara-stack-troubleshooting-runbook.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 平台工程师
- 远程顾问
estimated_read_time: 15min
intent_queries:
- 专有云 ESS 怎么配置
- 专有云 ACK 节点自动伸缩
- 专有云弹性伸缩失败排查
- 专有云伸缩组配额
trigger_keywords:
- ESS
- 弹性伸缩
- Cluster Autoscaler
- 伸缩组
- 配额
prerequisites:
- alicloud-basics
- k8s-autoscaling
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

# 专有云（Apsara Stack）- ESS 弹性伸缩

本文档面向在客户数据中心运维 [[18-云厂商/01-阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE，系统梳理专有云 ESS（弹性伸缩）的架构差异、伸缩组配置、触发策略、与 ACK Cluster Autoscaler 的集成及故障排查。

> **环境**: Apsara Stack 企业版/敏捷版

---

## 1. 专有云 ESS 架构差异

专有云 ESS 的底层依赖与公有云存在显著差异，主要体现在**资源交付的确定性**与**底层隔离性**上。

| 维度 | 专有云（Apsara Stack） | 公有云 |
|:---|:---|:---|
| **资源池** | 私有物理机集群（Compute Container） | 共享资源池 |
| **API 接入** | POP 网关（内网 VIP） | 公网/内网 OpenAPI |
| **伸缩限制** | 受限于物理资源配额（Quota） | 几乎无限扩展 |
| **付费模式** | 资源预存/内部计费 | 按量/包年包月 |
| **资源交付延迟** | 较高（受物理机库存约束） | 低（秒级） |
| **失败特征** | `InsufficientCapacity` 常见 | 较少 |

> **关键认知**：专有云弹性伸缩的瓶颈往往不是 ESS 本身，而是**物理资源池配额**。扩容失败时，优先检查 ASOP 中该租户的 ECS 配额。

---

## 2. 伸缩组（Scaling Group）配置

### 2.1 核心参数规划

| 参数 | 专有云配置建议 | 说明 |
|:---|:---|:---|
| **最小/最大实例数** | 匹配业务峰谷，留 20% 冗余 | 伸缩组维持实例数在 [min, max] |
| **期望实例数** | 匹配业务基准 | 伸缩组将维持实例数在此数值 |
| **移除策略** | `OldestInstance` | 优先回收存活最长的旧实例，降低滚动影响 |
| **后端服务器组** | 挂载 SLB/NLB | 自动将扩容实例加入 LB 后端 |
| **实例保护** | 启用核心实例保护 | 防止特定实例被意外缩容 |
| **冷却时间** | 300s（按业务调整） | 避免伸缩活动震荡 |
| **健康检查** | 启用 | 自动替换非 Running 的 ECS |

### 2.2 伸缩配置（Scaling Configuration）

```yaml
# 专有云伸缩配置要点（ASO/POP 创建时的关键参数）
ScalingConfiguration:
  ImageId: "m-apsara-xxx"           # 专有云自定义镜像（含 kubelet/CSI/Terway）
  InstanceType: "ecs.g7.2xlarge"    # 与 ACK 节点池规格一致
  SecurityGroupId: "sg-apsara-xxx"
  KeyPairName: "apsara-k8s-key"
  SystemDisk:
    Category: "cloud_essd"          # 专有云支持的盘类型
    Size: 120
  UserData: |                       # 节点启动脚本（注册到 ACK）
    #!/bin/bash
    # kubelet 注册逻辑（专有云 ACK 节点池通常自动处理）
```

> **镜像一致性**：伸缩配置使用的镜像必须与 ACK 节点池镜像一致，否则节点注册会失败。

---

## 3. 伸缩触发策略与健康检查

### 3.1 触发模式对比

| 模式 | 适用场景 | 专有云注意 |
|:---|:---|:---|
| **定时触发（Scheduled）** | 业务可预期的峰谷（早晚高峰） | 提前预留资源池配额 |
| **报警触发（Alarm）** | 基于 CPU/内存等指标 | 报警由 **CloudMonitor/天目** 推送到 ESS |
| **健康检查** | 自动替换异常实例 | 检查 ECS 状态是否 Running |

### 3.2 监控指标速查表

| 指标名称 | 单位 | 建议扩容阈值 | 建议缩容阈值 |
|:---|:---:|:---:|:---:|
| `CpuUtilization` | % | > 75% | < 30% |
| `MemoryUtilization` | % | > 80% | < 40% |
| `IntranetIn` | bits/s | 按业务带宽规划 | — |
| `IntranetOut` | bits/s | 按业务带宽规划 | — |
| `load_5m` | — | > 0.8 × 核数 | < 0.3 × 核数 |

> **指标来源**：专有云监控指标由天目（Tianmu）采集。确认天目 Agent 在 ECS 上正常运行，否则报警触发失效。

---

## 4. 与 ACK Cluster Autoscaler 集成

在专有云 ACK 环境中，节点弹性伸缩由 **Cluster-Autoscaler** 调用专有云 POP 接口实现，每个 ACK 节点池对应一个专有云 ESS 伸缩组。

### 4.1 集成架构

```
业务 Pod Pending
       ↓
Cluster-Autoscaler 监听 Pending Pod
       ↓
计算需要的节点规格/数量
       ↓
调用专有云 POP → ESS 修改伸缩组期望实例数
       ↓
ESS 创建 ECS（依赖伏羲资源池）
       ↓
ECS 启动 → 注册到 ACK 集群 → kubelet 注册节点
       ↓
Pod 调度到新节点
```

### 4.2 配置要点

| 配置项 | 说明 |
|--------|------|
| **ServiceAccount 授权** | `cluster-autoscaler` 需调用专有云 ESS 的 RAM 权限 |
| **节点池映射** | 每个 ACK 节点池 ↔ 一个专有云 ESS 伸缩组（ASG） |
| **资源配额** | ASOP 中租户 ECS 配额 ≥ 伸缩组 max |
| **POP Endpoint** | 配置专有云内网 POP 地址（VIP） |
| **镜像/UserData** | 与 ACK 节点池一致 |

### 4.3 验证 Cluster-Autoscaler 状态

```bash
# 🟢 低风险：只读
# CA Pod 状态
kubectl get pods -n kube-system | grep cluster-autoscaler
# CA 日志（看是否在调用 ESS/POP）
kubectl logs -n kube-cluster-autoscaler -l app=cluster-autoscaler --tail=100
# 节点池与伸缩组映射（ACK 节点池 annotation）
kubectl get nodepool -A -o yaml 2>/dev/null || echo "nodepool CRD 不存在，查 ASO 节点池"
# Pending Pod（触发伸缩的源头）
kubectl get pods -A --field-selector status.phase=Pending
```

---

## 5. 常见故障排查

### 5.1 扩容失败（资源不足）

| 项 | 内容 |
|----|------|
| **现象** | 伸缩活动显示 `InsufficientCapacity`；Pod 长期 Pending |
| **排查** | ASO 资源管理查租户 ECS 配额；查伏羲资源池水位 |
| **解决** | 申请扩充租户配额或联系平台管理员增加物理节点 |

```bash
# 🟢 低风险：只读
# ESS 伸缩活动状态（驻场 aliyun CLI）
aliyun ess DescribeScalingActivities --ScalingGroupId asg-xxx \
  --RegionId cn-apsara-local --endpoint ess.aliyuncs.com
# 节点 Pending 原因
kubectl describe pod <pending-pod> | grep -A15 Events
```

### 5.2 实例心跳超时

| 项 | 内容 |
|----|------|
| **现象** | 实例创建成功但无法加入伸缩组/集群 |
| **排查** | 镜像中 `aliyun-service` 守护进程是否启动；专有云网络连通性 |
| **解决** | 修复镜像 UserData；检查 POP 网络连通 |

### 5.3 伸缩组与节点池不一致

| 项 | 内容 |
|----|------|
| **现象** | ESS 弹出实例但 ACK 未注册；或 ACK 删除节点但 ESS 未缩容 |
| **排查** | CCM/CA 日志；节点 ProviderID 是否正确；RAM 权限 |
| **解决** | 修正 ProviderID 映射；确认 CA 的 RAM 权限 |

### 5.4 报警不触发

| 项 | 内容 |
|----|------|
| **现象** | CPU 已超阈值但未扩容 |
| **排查** | 天目 Agent 是否在运行；报警规则是否关联伸缩组；指标是否上报 |
| **解决** | 重启天目 Agent；检查报警规则与 ESS 关联 |

---

## 6. 伸缩运维最佳实践

1. **配额前置**：上线前在 ASOP 预分配足够 ECS 配额，避免峰时扩容失败
2. **镜像标准化**：伸缩配置镜像与 ACK 节点池镜像严格一致
3. **PodDisruptionBudget**：关键业务配 PDB，防止缩容驱逐导致不可用
4. **分批缩容**：缩容宜慢（冷却时间长），避免业务抖动
5. **监控大盘**：天目建立伸缩活动/节点数/Pending Pod 大盘，提前预警
6. **定期演练**：定期验证弹性伸缩链路（Pending→弹节点→调度）是否正常

---

## 相关文档

- [[18-云厂商/01-阿里云/02-ACK集群运维.md|02 ACK集群运维]]
- [[18-云厂商/01-阿里云/专有云-Apsara/004-apsara-stack-pop-operations.md|252 POP 平台运维（ASOP）]]
- [[18-云厂商/01-阿里云/专有云-Apsara/005-apsara-tianji-aso-operations.md|253 天基/ASO 运维]]
- [[18-云厂商/01-阿里云/专有云-Apsara/01-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]

## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]]
- [[17-系统基础/05-速查卡/k8s.md|k8s]]

<!-- risk-assessed -->
