---
title: 第九课：HPA - 自动伸缩 [08-scaling]
description: 2. 掌握 HPA 的配置方法
summary: 2. 掌握 HPA 的配置方法
category: k8s-lecturer
tags:
- k8s
- training
- lecturer
- hpa
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第九课：HPA - 自动伸缩 是什么
- 如何 第九课：HPA - 自动伸缩
trigger_keywords:
- 第九课：HPA
- 自动伸缩
- k8s
- lecturer
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第九课：HPA - 自动伸缩

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 HPA 的概念和作用
2. 掌握 HPA 的配置方法
3. 了解基于 CPU 和内存的扩缩容
4. 学会排查 HPA 不工作的问题

---

## 1. HPA 问题引入

### 1.1 问题场景

```
【场景】

你的电商网站平时只需要 3 个 Pod 处理请求。
但到了双十一促销，大批用户涌入，3 个 Pod 完全不够用！
手动扩容太慢，等你扩好，活动都结束了。

问题：如何让应用自动应对流量变化？

【解决方案】

HPA (Horizontal Pod Autoscaler)！

HPA 会：
• 监控 Pod 的 CPU/内存使用率
• 当使用率超过阈值时，自动增加 Pod 数量
• 当使用率降低时，自动减少 Pod 数量

就像：
• 餐厅的智能叫号系统
• 排队人太多就多开几个窗口
• 排队人少了就关闭多余窗口
• 完全自动，无需人工干预
```

### 1.2 HPA 类比

```
【餐厅类比】

HPA = 餐厅的智能叫号系统
Pod = 服务窗口
CPU/内存 = 顾客排队数量

• 窗口（Pod）处理顾客请求
• 如果排队人数（CPU/内存）太多，系统自动开新窗口
• 如果排队人数太少，系统自动关闭多余的窗口
• 始终保持合理的工作效率

【[[entities/kubernetes.md|k8s]] 类比】

HPA = 水平 Pod 自动伸缩器
Deployment = 被伸缩的资源
Metrics Server = 监控系统，收集使用率数据
```

---

## 2. HPA 配置

### 2.1 基于 CPU 的 HPA

```
【YAML 示例】

apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2        # 最少 2 个 Pod
  maxReplicas: 10       # 最多 10 个 Pod
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 80  # CPU 使用率超过 80% 时扩容

【解释】

• scaleTargetRef：指定要伸缩的 Deployment
• minReplicas：最小副本数
• maxReplicas：最大副本数
• averageUtilization：CPU 使用率目标值
```

### 2.2 基于内存的 HPA

```
【YAML 示例】

apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80  # 内存使用率超过 80% 时扩容
```

### 2.3 同时基于 CPU 和内存

```
【YAML 示例】

apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

### 2.4 命令行创建

```
# 🟢 低风险：只读/信息收集，通常无副作用
【快速创建 HPA】

kubectl autoscale deployment my-app \
  --cpu-percent=80 \
  --min=2 \
  --max=10

【查看 HPA】

kubectl get hpa

【查看 HPA 详情】

kubectl describe hpa my-app-hpa
```
---

## 3. HPA 原理

### 3.1 工作流程

```
【HPA 工作原理】

1. Metrics Server 定期采集 Pod 的资源使用数据
2. HPA Controller 检查使用率是否超过目标
3. 如果超过，计算需要的副本数
4. 通过 Deployment 更新副本数
5. Deployment 创建或删除 Pod

【副本数计算公式】

desiredReplicas = ceil(currentReplicas * currentMetricValue / targetMetricValue)

例如：
• 当前副本：3
• 当前 CPU：80%
• 目标 CPU：80%
• 需要的副本：ceil(3 * 80 / 80) = 3

例如（需要扩容）：
• 当前副本：3
• 当前 CPU：160%
• 目标 CPU：80%
• 需要的副本：ceil(3 * 160 / 80) = 6
```

### 3.2 冷却机制

```
【冷却机制】

为了防止频繁扩缩容，HPA 有冷却机制：

spec:
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 扩容冷却 5 分钟
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
    scaleUp:
      stabilizationWindowSeconds: 0   # 缩容立即生效
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
      - type: Pods
        value: 4
        periodSeconds: 15
```

---

## 4. 常见问题

### 4.1 HPA 不触发扩容

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查 HPA 是否存在
   kubectl get hpa

2. 检查 HPA 状态
   kubectl describe hpa <name>

   看 Conditions 部分：
   • AbleToScale = True → 可以伸缩
   • ScalingActive = True → 伸缩功能正常
   • ScalableConditionReason = "ReadyPSA" → 检查 Pod 是否有资源请求

3. 检查 Metrics Server
   kubectl get pods -n kube-system -l k8s-app=k8s-dashboard-metrics-server

   Metrics Server 必须正常运行！

4. 检查 Pod 的资源请求
   HPA 需要 Pod 设置了 resources.requests 才能计算使用率！

   kubectl describe pod <pod-name> | grep -A5 "Requests"
```
### 4.2 HPA 一直处于 Scaling 状态

```
【原因】

HPA 在等待冷却时间结束，或者正在执行扩缩容操作。

【解决方案】

等待几分钟，HPA 会自动完成。
如果长时间未完成，检查：
1. Deployment 是否能成功创建新 Pod
2. 集群是否有足够的资源
3. 是否有调度问题
```

### 4.3 如何验证 HPA 工作

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【测试方法】

1. 创建 HPA
   kubectl autoscale deployment my-app --cpu-percent=50 --min=2 --max=5

2. 查看初始状态
   kubectl get hpa

3. 增加负载
   kubectl run -it --rm load-generator --image=busybox -- /bin/sh
   # 在容器内执行
   while true; do wget -q -O- http://my-app; done

4. 观察 HPA 变化
   watch kubectl get hpa

5. 停止负载，观察缩容
```
---

## 5. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【命令速查】

创建 HPA：
kubectl autoscale deployment my-app --cpu-percent=80 --min=2 --max=10

查看 HPA：
kubectl get hpa

查看 HPA 详情：
kubectl describe hpa <name>

删除 HPA：
kubectl delete hpa <name>

更新 HPA：
kubectl edit hpa <name>

【核心要点】

1. HPA 自动调整 Pod 副本数
2. 基于 CPU/内存使用率判断是否扩容
3. 需要 Metrics Server 提供监控数据
4. Pod 必须设置 resources.requests
5. 有冷却机制防止频繁扩缩容

【下节课预告】

下节课我们会学习常见问题排查：
• Pod 问题（Pending、CrashLoopBackOff）
• 网络问题（Service 无法访问、DNS 解析失败）
• 资源问题（配额超限、OOM）

有问题吗？"
```
---

**关联文档**:
- [../09-troubleshooting/09-common-problems.md](../09-troubleshooting/09-common-problems.md) — 常见问题排查
- [../../domain-10-troubleshooting-diagnostics/topic-skills/07-hpa-scaling-failure.md](../../domain-10-troubleshooting-diagnostics/技能体系/07-hpa-scaling-failure.md) — HPA 问题 [[SKILL|Skill]]
- [../../domain-02-workloads-applications/](../../domain-02-workloads-applications/) — 工作负载文档


<!-- risk-assessed -->
