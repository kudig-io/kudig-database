---
title: 混沌工程（Chaos Engineering）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kafka
- hpa
- statefulset
- ingress
- rbac
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌工程（Chaos Engineering） 是什么
- 如何 混沌工程（Chaos Engineering）
trigger_keywords:
- 混沌工程
- Chaos
- Engineering
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 混沌工程（Chaos Engineering）

## 概述

**混沌工程**是一种通过在生产环境中有控制地注入问题，来验证系统韧性和发现潜在弱点的工程实践。其核心理念是"**主动制造问题，以避免被动承受问题**"。2026 年的 [[Kubernetes|Kubernetes]] 生产环境中，混沌工程已成为 SRE 成熟度模型中的关键能力，主流工具包括 **[[Litmus|Litmus]]、Chaos Mesh、Gremlin** 和 Netflix 开源的 **Chaos Monkey**。

## 核心概念/原理

### 1. 混沌工程五大原则

根据 Principles of Chaos Engineering：
1. **建立稳态假设**：定义系统在正常运行时的可观测行为（如 P99 延迟 < 200ms、错误率 < 0.1%）
2. **注入真实世界事件**：模拟服务器崩溃、网络分区、磁盘满、CPU 满载、依赖服务超时等
3. **生产环境运行**：只有在真实生产流量下验证，才能反映真实的系统行为
4. **自动化并持续运行**：将混沌实验集成到 CI/CD 或定期运行的 Pipeline 中
5. **最小化爆炸半径**：通过 canary namespace、shadow traffic 等方式控制实验影响范围

### 2. 故障注入类型

| 问题类型 | 说明 | 典型场景 |
|----------|------|----------|
| **Pod Failure** | 随机杀死 Pod | 验证 Deployment 自愈、HPA 响应 |
| **Node Failure** | 关闭/重启工作节点 | 验证 [[StatefulSet|StatefulSet]] 高可用、Pod 迁移 |
| **Network Latency** | 增加网络延迟或丢包 | 验证超时、重试、熔断策略 |
| **Network Partition** | 隔离 Pod/节点间的网络 | 验证分布式共识（etcd、Kafka、ZooKeeper）|
| **CPU/Memory Stress** | 耗尽节点或 Pod 的资源 | 验证资源限制、OOM 行为、降级策略 |
| **Disk Failure / Fill** | 模拟磁盘满或 I/O 错误 | 验证日志轮转、存储告警、应用容错 |
| **DNS Chaos** | 篡改或延迟 DNS 响应 | 验证服务发现容错、本地缓存 |
| **Time Chaos** | 修改系统时间 | 验证证书过期、定时任务、分布式锁 |

### 3. GameDay 文化

**GameDay** 是有组织的混沌工程演练活动：
- **目标明确**：本次 GameDay 要验证什么假设？（如"单个可用区问题不会影响支付服务"）
- **团队参与**：开发、SRE、运维、产品经理共同参与观察和复盘
- **事中监控**：实时观察关键指标是否偏离稳态
- **事后复盘**：无论结果如何，都进行详细的 5Why 分析并制定改进项

## 关键机制或特性

### Litmus Chaos

**Litmus** 是 CNCF 孵化项目，Kubernetes 原生的混沌工程框架：
- **ChaosExperiment**：定义具体的故障注入实验（如 pod-delete、node-cpu-hog）
- **ChaosEngine**：将实验绑定到目标应用/Namespace
- **ChaosResults**：记录每次实验的结果和系统稳态变化
- **Hub**：提供预构建的 100+ 个混沌实验模板

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: nginx-chaos
spec:
  appinfo:
    appns: 'default'
    applabel: 'app=nginx'
    appkind: 'deployment'
  chaosServiceAccount: litmus-admin
  experiments:
    - name: pod-delete
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '30'
            - name: CHAOS_INTERVAL
              value: '10'
```

### Chaos Mesh

**Chaos Mesh** 是 PingCAP 开源的混沌工程平台，以可视化和丰富的问题类型著称：
- **Dashboard**：通过 Web UI 设计、运行和监控混沌实验
- **Workflow**：支持编排多步骤复杂的混沌场景
- **Security Model**：基于 RBAC 控制谁可以执行哪些实验

### 稳态监控与自动终止

混沌实验必须配置**自动终止条件（Abort Conditions）**：
- 当错误率超过阈值时立即停止注入
- 当 P99 延迟超过 SLO 时自动回滚
- 当关键业务指标（如订单量）下降时触发熔断

## 使用场景

1. **验证自动扩缩容**：注入 CPU stress，观察 HPA 是否能在 60 秒内扩容新 Pod
2. **验证有状态服务高可用**：杀死 ZooKeeper/Kafka/Raft Leader Pod，观察选举和恢复时间
3. **验证微服务熔断**：对下游依赖注入高延迟，验证上游是否正确触发 Circuit Breaker
4. **验证跨区域灾备**：模拟整个可用区的网络分区，验证流量是否自动切换到备用区域
5. **验证新发布稳定性**：在金丝雀环境中对新版本进行混沌实验，比生产环境提前暴露缺陷

## 最佳实践/注意事项

- **从staging开始，逐步推向生产**：先在预发环境建立信心，再在生产的小范围 Canary 中运行
- **定义清晰的稳态指标**：没有指标就没有混沌工程，必须事先定义可量化的"正常"标准
- **最小权限原则**：混沌工程工具本身具有破坏性，应使用独立的服务账号并严格限制权限
- **工作时间运行**：不要在深夜无人值守时运行高风险实验，确保关键人员在线
- **预先沟通**：生产实验前通知相关团队，避免在发布窗口、促销高峰等敏感期执行
- **记录和复盘**：每次实验都应像事故一样记录时间线、观察结果和改进措施
- **自动化常态化**：将混沌实验纳入 CI/CD 或每周定时任务，而不是一年一度的"运动式"演练
- **渐进式增加复杂度**：从单 Pod 问题开始，逐步过渡到多节点、网络分区、级联问题

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| ChaosEngine 状态为 Failed | Litmus ServiceAccount 权限不足 | `kubectl describe chaosengine <name>` | 检查 `chaosServiceAccount` 的 RBAC 权限 |
| 混沌实验未注入到目标 Pod | Label selector 不匹配 | `kubectl get pods -l <applabel>` | 确认 `appinfo.applabel` 与目标 Pod 匹配 |
| 实验后服务未恢复 | abort condition 未配置或 cleanup 失败 | `kubectl get chaosresult <name> -o yaml` | 手动清理实验残留资源 |
| Chaos Mesh Dashboard 不可用 | chaos-dashboard Service 未暴露 | `kubectl get svc -n chaos-mesh` | 配置 Ingress 或 port-forward 访问 |
| 网络延迟注入无效果 | tc/iptables 权限不足（缺少 NET_ADMIN） | `kubectl logs -n chaos-mesh chaos-daemon-*` | 确认 chaos-daemon 以 privileged 模式运行 |
| 实验超出爆炸半径 | namespace selector 过宽 | 查看 ChaosExperiment 的 scope 配置 | 限制实验范围到特定 namespace 和 label |

## 生产检查清单

- [ ] 混沌工程工具使用独立的 ServiceAccount，最小权限
- [ ] 所有实验配置了自动终止条件（abort conditions）
- [ ] 生产实验前已在 staging 环境验证
- [ ] 实验期间有值班人员在线监控
- [ ] 已与相关团队沟通实验计划和时间窗口
- [ ] 稳态指标（P99 延迟、错误率）已定义和监控
- [ ] 每次实验都有详细的复盘报告
- [ ] GameDay 演练已纳入季度计划

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Litmus: 查看混沌实验状态
kubectl get chaosengine -A
kubectl get chaosresult -A

# Litmus: 取消正在运行的实验
kubectl delete chaosengine <name> -n <namespace>

# Chaos Mesh: 查看所有混沌实验
kubectl get podchaos,networkchaos,iochaos,stresschaos -A

# Chaos Mesh: 暂停实验
kubectl annotate podchaos <name> experiment.chaos-mesh.org/pause=true

# 端口转发 Chaos Mesh Dashboard
kubectl port-forward -n chaos-mesh svc/chaos-dashboard 2333:2333

# 检查稳态指标（PromQL 示例）
# histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le))
```
## 交叉引用

- [Principles of Chaos Engineering](https://principlesofchaos.org/)
- [Litmus Chaos Documentation](https://docs.litmuschaos.io/)
- [Chaos Mesh Documentation](https://chaos-mesh.org/docs/)
- 相关主题：[Disruptions](../workloads/disruptions.md) · [Pod Lifecycle](../workloads/pod-lifecycle.md) · [Horizontal Pod Autoscaling](../workloads/horizontal-pod-autoscaling.md)

## 参考链接

- [Chaos Engineering]()

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
