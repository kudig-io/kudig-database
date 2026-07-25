---
title: 混沌工程与灾备演练的结合
description: → 管理层参与
summary: → 管理层参与
category: synthesis
tags:
- chaos-engineering
- disaster-recovery
- game-day
- reliability
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌工程与灾备演练的结合 是什么
- 如何 混沌工程与灾备演练的结合
trigger_keywords:
- 混沌工程与灾备演练的结合
prerequisites:
- kubectl-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 混沌工程与灾备演练的结合

## 概述

混沌工程与灾备演练的结合，形成了从日常自动化验证到季度大规模演练的完整弹性保障体系。混沌工程通过主动注入故障来验证系统的自愈能力，灾备演练则验证业务在极端场景下的恢复能力。两者融合后，将"假设系统能恢复"变为"已验证系统能恢复"。

## 分层验证体系

### 频率与范围矩阵

```
日常 (Daily):
  → 自动化混沌实验（小范围，单 Pod/单节点级别）
  → 验证自愈能力（重启、重新调度）
  → 持续验证 SLO（错误预算消耗）
  → 工具: Chaos Mesh CronJob

周度 (Weekly):
  → 有计划的中等规模实验（多 Pod、网络延迟）
  → 验证故障转移流程（HPA 扩容、Failover）
  → 团队轮换 On-call 响应
  → 工具: Chaos Mesh Workflow

月度 (Monthly):
  → 跨服务依赖问题实验（级联故障）
  → 验证灾难恢复手册（DR Playbook）
  → 模拟 AZ 级别故障
  → 工具: Chaos Mesh + 自动化验证脚本

季度 (Quarterly):
  → 全面 GameDay
  → 生产环境大规模演练（Region 级故障）
  → 管理层参与，跨部门协作
  → 工具: 全链路混沌 + 人工决策
```

## GameDay 流程

GameDay 是最完整的混沌演练形式，模拟真实的大规模故障场景：

```
1. 场景设定
   → "Region A 完全不可用"
   → 定义影响范围和演练目标
   → 确定成功标准（RTO/RPO/SLO 恢复时间）

2. 注入问题
   → Chaos Mesh 网络分区
   → 模拟 DNS 问题
   → Pod 批量删除
   → 节点驱逐
   → 数据库故障注入

3. 团队响应
   → 执行 DR Playbook
   → 流量切换到 Region B
   → 通信协调（Slack 专用频道）
   → 决策记录

4. 验证恢复
   → SLO 达标
   → 业务功能正常（自动化测试）
   → 数据一致性验证

5. 复盘改进
   → 更新 Playbook
   → 修复发现的问题
   → 优先级排序和跟踪
```

## 技术实现：Chaos Mesh

### 自动化混沌实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-kill-daily
  namespace: chaos-testing
spec:
  action: pod-kill                   # 故障类型
  mode: one                           # 每次影响一个 Pod
  selector:
    namespaces:
      - production
    labelSelectors:
      chaos-test: "enabled"           # 仅影响标记的 Pod
  scheduler:
    cron: "@daily"                    # 每日执行
```

### 网络分区实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-partition-az
spec:
  action: partition
  mode: all
  selector:
    namespaces:
      - production
    labelSelectors:
      topology.kubernetes.io/zone: us-east-1a
  direction: both
  target:
    selector:
      namespaces:
        - production
      labelSelectors:
        topology.kubernetes.io/zone: us-east-1b
    mode: all
  duration: "5m"
```

### 混沌实验自动化验证

```bash
# 🟡 中风险：会修改状态，需控制 blast radius
# 定义实验 + 自动化验证
chaosctl verify --experiment network-partition-az \
  --check "curl -f http://api/health" \
  --check "prometheus_query(up{job='api'} == 1)" \
  --duration 5m
```

## 与 SLO 集成

混沌实验必须与 SLO 监控联动，确保实验期间业务影响可控：

```
实验前:
  → 检查 SLO 错误预算余额
  → 余额不足时禁止实验（避免影响 SLO 达标）

实验中:
  → 实时监控 SLI 指标
  → SLI 突破阈值时自动终止实验

实验后:
  → 验证 SLI 恢复
  → 记录 SLO 预算消耗
```

## 最佳实践

- **从小范围开始**：先在 staging 环境运行单 Pod kill 实验，验证自愈机制后再逐步扩大到多 AZ/Region 级别
- **设置 blast radius 控制**：通过 namespace/label 选择器限制实验影响范围，避免波及未准备的服务
- **自动化验证取代人工检查**：实验后用自动化脚本验证业务功能，而非依赖人工点击测试
- **每次 GameDay 都要有明确目标**：不是为了制造混乱，而是验证特定假设（"Region 故障后 RTO < 15 分钟"）
- **建立复盘跟踪机制**：GameDay 发现的问题必须有 owner 和修复 deadline，否则演练失去意义

## 常见陷阱

- **生产环境实验无 abort 机制**：实验失控时没有快速终止手段——必须配置自动 abort（基于 SLI 阈值）和人工 abort 按钮
- **混沌实验影响未标记的服务**：selector 配置不当导致意外 Pod 被注入故障——需要严格的 label 管理
- **GameDay 变成表演**：如果每次都是预排练的脚本，无法发现真实问题——需要引入随机性和真实故障模式

## 相关 Domain

- [[12-可靠性/04-混沌工程/01-chaos-engineering-overview.md|01 chaos engineering overview]]
- [[12-可靠性/02-灾难恢复/01-dr-scenarios-catalog.md|01 dr scenarios catalog]]
- [[12-可靠性/07-性能测试/02-chaos-load-integration.md|02 chaos load integration]]

## 相关页面

- [[22-概念/06-可观测性/slo-monitoring-integration.md|SLO 与监控集成]] — 混沌实验的 SLO 保障
- [[22-概念/04-存储/data-protection-k8s.md|K8s 数据保护]] — 灾备恢复基础


<!-- risk-assessed -->
