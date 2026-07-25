---
title: Kubernetes 容量规划框架
description: 面向阿里云/专有云 K8s 的容量规划框架，涵盖指标体系、趋势预测、容量决策流程与成本优化。
summary: 面向阿里云/专有云 K8s 的容量规划框架，涵盖指标体系、趋势预测、容量决策流程与成本优化。
category: reliability
tags:
- k8s
- capacity-planning
- forecasting
- cost-optimization
- prometheus
- metrics
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 20min
intent_queries:
- K8s 容量规划框架
- 容量规划指标体系
- 阿里云 K8s 容量预测与决策
trigger_keywords:
- 容量规划
- capacity planning
- forecasting
- 预测
- 成本优化
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- sre-practices
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




# Kubernetes 容量规划框架

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，建立容量规划的方法论、指标体系、预测方法与决策流程。

## 目录

1. [容量规划概述](#容量规划概述)
2. [指标体系](#指标体系)
3. [现状评估](#现状评估)
4. [趋势预测方法](#趋势预测方法)
5. [容量决策流程](#容量决策流程)
6. [成本优化策略](#成本优化策略)
7. [阿里云/专有云场景](#阿里云专有云场景)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 容量规划概述

### 1.1 目标与价值

容量规划的核心目标是：在保障业务稳定性的前提下，以最低成本满足未来资源需求。

| 维度 | 说明 |
|:---|:---|
| 业务连续性 | 避免资源不足导致的服务降级 |
| 成本控制 | 避免过度预留造成浪费 |
| 交付效率 | 提前准备资源，缩短扩容周期 |
| 风险可控 | 识别瓶颈，提前消除单点 |

### 1.2 规划周期

| 周期 | 关注点 | 输出 |
|:---|:---|:---|
| 短期（1-4 周） | 告警阈值、HPA 调整 | 扩容工单 |
| 中期（1-3 月） | 节点池规划、预留实例 | 采购计划 |
| 长期（半年-1 年） | 架构演进、混合云 | 预算与路线图 |

---

## 2. 指标体系

### 2.1 核心指标分层

| 层级 | 指标 | 来源 |
|:---|:---|:---|
| 集群层 | CPU/内存分配率、实际使用率、节点数 | Prometheus + metrics-server |
| 节点层 | CPU/内存/磁盘/网络使用率、压力条件 | node-exporter |
| Pod 层 | requests/limits 使用率、重启次数 | kube-state-metrics |
| 应用层 | QPS、延迟、队列长度、连接数 | 应用自身指标 |
| 业务层 | 订单量、DAU、任务数 | 业务系统 |

### 2.2 关键容量指标

```bash
# 集群 CPU 分配率
sum(kube_pod_container_resource_requests{resource="cpu"}) / sum(kube_node_status_allocatable{resource="cpu"})

# 集群内存分配率
sum(kube_pod_container_resource_requests{resource="memory"}) / sum(kube_node_status_allocatable{resource="memory"})

# 节点实际 CPU 使用率
100 - avg(irate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance) * 100

# Pod OOM 频率
increase(kube_pod_container_status_restarts_total[1h])
```

---

## 3. 现状评估

### 3.1 集群资源盘点脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# cluster-capacity-audit.sh
# 用途：快速盘点集群资源现状，输出容量基线报告

echo "=== 集群容量盘点报告 ==="
echo "时间: $(date)"

# 节点总数与状态
echo -e "\n[节点状态]"
kubectl get nodes -o wide

# 资源容量
echo -e "\n[集群资源容量]"
kubectl describe nodes | grep -E "(Name|Capacity|Allocatable|cpu|memory|ephemeral-storage)" | head -40

# 实际使用率
echo -e "\n[节点实际使用率]"
kubectl top nodes

# Pod 资源请求
echo -e "\n[命名空间资源请求 Top 10]"
kubectl top pods --all-namespaces --sort-by=cpu | head -10

# PVC 使用
echo -e "\n[PVC 使用]"
kubectl get pvc --all-namespaces | awk '{print $1,$2,$3,$4}' | column -t
```
### 3.2 容量基线表

| 资源 | 总容量 | 已分配 | 分配率 | 实际使用 | 使用率 |
|:---|---:|---:|---:|---:|---:|
| CPU | 1000 core | 750 core | 75% | 350 core | 35% |
| 内存 | 4000 Gi | 2800 Gi | 70% | 1200 Gi | 30% |
| Pod | 10000 | 6500 | 65% | - | - |
| PVC | 100 Ti | 60 Ti | 60% | 35 Ti | 35% |

---

## 4. 趋势预测方法

### 4.1 基于 Prometheus 的历史数据

```bash
# 查询过去 30 天 CPU 平均使用率
START=$(date -u -d "30 days ago" +%s)
END=$(date -u +%s)
curl -G "http://prometheus:9090/api/v1/query_range" \
  --data-urlencode "query=avg(100 - irate(node_cpu_seconds_total{mode='idle'}[5m]) * 100)" \
  --data-urlencode "start=${START}" \
  --data-urlencode "end=${END}" \
  --data-urlencode "step=1h"
```

### 4.2 简单线性预测脚本

```python
#!/usr/bin/env python3
# capacity-forecast.py
# 用途：基于最近 N 天 CPU 使用率线性预测未来 30 天

import json
import numpy as np
from datetime import datetime, timedelta

def forecast_usage(historical_values, days_ahead=30):
    x = np.arange(len(historical_values))
    y = np.array(historical_values)
    coefficients = np.polyfit(x, y, 1)
    trend = np.poly1d(coefficients)
    future = [trend(len(historical_values) + i) for i in range(days_ahead)]
    return future

# 示例：最近 7 天平均 CPU 使用率
weekly_cpu = [35.2, 36.1, 38.5, 40.2, 42.8, 45.1, 47.3]
predicted = forecast_usage(weekly_cpu)
print(f"30 天后预测 CPU 使用率: {predicted[-1]:.2f}%")
```

### 4.3 业务驱动预测

| 业务指标 | 资源换算关系 | 示例 |
|:---|:---|:---|
| 每单 CPU | 0.5m core / order | 日单量 100W → 500m core |
| 每用户内存 | 10 KiB / DAU | DAU 100W → 10 GiB |
| 每任务磁盘 | 100 MB / task | 日任务 10W → 10 TB |

---

## 5. 容量决策流程

### 5.1 决策树

```
实际使用率 > 70% 或 分配率 > 85%
    │
    ├─ 短期波动 → 调整 HPA / VPA
    │
    ├─ 持续增长 → 扩容节点池 / 升配
    │
    ├─ 资源碎片化 → 调度优化 / 规整化
    │
    └─ 使用率 < 30% → 缩容 / 降配
```

### 5.2 决策会议模板

| 议题 | 内容 | 责任人 |
|:---|:---|:---|
| 现状回顾 | 分配率、使用率、告警 | SRE |
| 趋势预测 | 未来 4 周资源需求 | 平台工程师 |
| 扩容方案 | 节点规格、数量、可用区 | 架构师 |
| 成本评估 | 按量/包年包月/预留实例 | 财务/采购 |
| 执行计划 | 变更窗口、回滚方案 | 变更经理 |

---

## 6. 成本优化策略

### 6.1 资源利用率优化

| 策略 | 说明 | 预期收益 |
|:---|:---|---:|
| requests 调优 | 按实际使用设置 requests | 10-20% |
| VPA 自动推荐 | 基于历史自动调整 | 15-25% |
| 潮汐调度 | 离线任务填充低谷 | 20-30% |
| Spot/抢占式实例 | 非核心负载使用 | 50-70% |
| 包年包月 | 基线负载使用 | 30-50% |

### 6.2 阿里云成本分析

```bash
# 使用 aliyun CLI 查询实例费用
aliyun ecs DescribeInstances --RegionId cn-hangzhou

# 查询账单
aliyun bssopenapi QueryAccountBill --BillingCycle 2026-06
```

---

## 7. 阿里云/专有云场景

### 7.1 ACK 节点池规划

```yaml
apiVersion: autoscaling.alibabacloud.com/v1beta1
kind: NodePool
metadata:
  name: production-general
spec:
  clusterID: <cluster-id>
  scalingGroup:
    instanceTypes:
      - ecs.g7.xlarge
      - ecs.g7.2xlarge
    minSize: 3
    maxSize: 50
    scalingPolicy: release
    systemDiskCategory: cloud_essd
    systemDiskSize: 120
    vswitchIDs:
      - vsw-xxx1
      - vsw-xxx2
      - vsw-xxx3
```

### 7.2 专有云容量管理

- 物理机资源池由 ASO/天基统一管理
- 扩容需提前申请物理机上架
- 建议保留 20% 以上的资源缓冲

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 容量基线 | 每月更新 | 容量报告 |
| 预测模型 | 持续校准 | 预测准确率 |
| 扩容阈值 | 分配率 85%、使用率 70% | 告警规则 |
| 成本看板 | 按部门/项目拆分 | 阿里云账单 |
| 资源标签 | 成本归属清晰 | `kubectl get nodes --show-labels` |
| 演练验证 | 每季度模拟扩容 | 演练报告 |
| 回退机制 | 缩容方案 | 变更手册 |

---

## 容量规划组织流程

容量规划不仅是技术工作，也需要跨团队协作。建议成立由 SRE、FinOps、业务代表组成的容量委员会，定期评审并决策。

### 角色分工

| 角色 | 职责 |
|:---|:---|
| SRE | 采集指标、建立基线、输出扩容方案 |
| FinOps | 成本分摊、预算审批、优化建议 |
| 业务代表 | 提供增长预测、活动计划、优先级 |
| 平台工程师 | 执行扩容、优化调度与资源配额 |

### 风险与应急预案

| 风险 | 应对 |
|:---|:---|
| 业务峰值远超预测 | 启用紧急扩容流程，提前预留缓冲节点 |
| 云厂商资源不足 | 多可用区/多实例规格备选 |
| 成本超预算 | 启用 Spot 实例、Rightsizing、缩容 |
| 数据丢失 | 备份与快照策略纳入容量规划 |

### 容量评审会议模板

1. 上月资源使用与成本回顾
2. 下月业务增长与活动输入
3. 当前容量缺口与风险识别
4. 扩容/缩容决策与责任人
5. 决策落地跟踪与下次 review 时间

## 容量规划与业务规划对齐

容量规划不能脱离业务节奏。建议在每年 Q4 与业务部门共同制定下一年度容量预算，并在每季度根据实际增长调整。

### 业务输入清单

| 输入项 | 说明 |
|:---|:---|
| 用户增长预测 | 新增 DAU/MAU 对资源的影响 |
| 营销活动 | 大促、秒杀、拉新活动的峰值预估 |
| 产品发布 | 新功能上线带来的流量变化 |
| 数据增长 | 存储、日志、指标数据增长 |
| 合规要求 | 数据保留期限、异地备份带来的容量需求 |

### 容量预算审批流程

1. SRE 输出容量需求报告
2. FinOps 评估成本与预算
3. 业务方确认增长假设
4. 管理层审批预算
5. 平台工程师执行扩容
6. 效果监控与复盘

## 典型工单场景与处理

**场景**：业务方反馈大促期间应用频繁 Pending。

处理步骤：
1. 查看 Pending Pod 的 Events，确认是否为资源不足。
2. 使用 Prometheus 分析历史峰值与节点利用率。
3. 临时扩容节点池或调整 HPA maxReplicas。
4. 大促后回收临时资源并 rightsizing。
5. 将大促容量需求纳入下季度容量规划。

## Related

- [[domain-09-reliability-engineering/容量规划/24-capacity-planning-forecasting.md|容量规划与预测]]
- [[domain-09-reliability-engineering/容量规划/25-ai-driven-capacity-planning-cost-optimization-2025.md|AI 驱动的容量规划与成本优化]]

## See Also

- [[domain-06-observability/指标/01-prometheus-enterprise-monitoring.md|Prometheus 企业监控]]
- [[domain-07-platform-engineering/99-karpenter-node-autoscaling-guide.md|Karpenter 节点自动扩缩容指南]]


<!-- risk-assessed -->
