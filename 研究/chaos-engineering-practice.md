---
title: K8s 混沌工程落地实践研究
summary: 深入研究混沌工程在 Kubernetes 生产环境的落地方法论，覆盖实验设计、爆炸半径控制、自动化流水线和组织文化。
category: research
tags:
- research
- chaos-engineering
- reliability
- chaos-mesh
- litmus
- game-day
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# K8s 混沌工程落地实践研究

## 研究背景

混沌工程（Chaos Engineering）是通过主动注入故障来验证系统弹性的工程实践。Netflix 的 Chaos Monkey 开创了这一领域，但在 Kubernetes 生产环境中落地混沌工程面临独特挑战：

- **爆炸半径控制**：实验失控可能影响真实用户
- **实验设计复杂**：K8s 组件交互关系复杂，实验设计需要深入理解
- **自动化难度高**：实验需要与 CI/CD 和 SLO 集成
- **文化阻力**：开发团队担心实验导致故障

## 核心问题

1. 混沌实验的科学设计方法（假设→注入→观测→结论）是什么？
2. Chaos Mesh vs Litmus 在 K8s 混沌工程中的能力对比？
3. 如何将混沌实验与 SLO 和 CI/CD 管道集成？
4. 爆炸半径（Blast Radius）控制的分层策略？

## 调研发现

### 发现一：混沌实验科学方法

```
Step 1: 提出假设
  → "当 payment Pod 副本减少 50% 时，SLO 不会违反"

Step 2: 定义稳态
  → P99 延迟 < 500ms，错误率 < 0.1%，QPS > 1000

Step 3: 注入故障
  → 终止 50% payment Pod

Step 4: 观测系统
  → 监控 SLO 指标变化
  → 记录恢复时间

Step 5: 验证假设
  → 系统是否保持在稳态？
  → 如果是 → 弹性已验证
  → 如果否 → 发现改进机会

Step 6: 扩大范围
  → 逐步增加爆炸半径
  → 从单 Pod → 多 Pod → 节点 → AZ → 区域
```

### 发现二：混沌实验类型矩阵

| 故障类型 | Chaos Mesh 实验 | 验证能力 | 生产安全等级 |
|---------|----------------|---------|------------|
| **Pod 故障** | PodKill/PodChaos | 副本弹性、重调度 | 🟢 |
| **网络延迟** | NetworkChaos-delay | 超时处理、重试 | 🟡 |
| **网络丢包** | NetworkChaos-loss | 重试、熔断 | 🟡 |
| **网络分区** | NetworkChaos-partition | 降级、故障转移 | 🔴 |
| **CPU 压力** | StressChaos-cpu | 限流、扩容 | 🟡 |
| **内存压力** | StressChaos-mem | OOM 处理、驱逐 | 🔴 |
| **磁盘 IO** | IOChaos | I/O 超时处理 | 🟡 |
| **时间偏移** | TimeChaos | 证书过期、定时任务 | 🔴 |
| **DNS 故障** | DNSChaos | DNS fallback | 🟡 |

### 发现三：Chaos Mesh vs Litmus

| 维度 | Chaos Mesh | Litmus Chaos |
|------|-----------|-------------|
| **开发者** | CNCF (CNCF Incubating) | Harness (CNCF Incubating) |
| **架构** | CRD + Controller | CRD + Workflow (Argo) |
| **UI** | Chaos Dashboard | ChaosCenter |
| **GitOps** | ✅ CRD 原生 | ✅ CRD + GitOps |
| **实验范围** | K8s 原生 | K8s + VM + 云 |
| **社区活跃** | ⬤⬤⬤⬤ | ⬤⬤⬤⬤ |
| **推荐场景** | K8s 原生混沌 | 多平台混沌 |

### 发现四：自动化混沌流水线

```yaml
# CI/CD 集成的自动化混沌实验
apiVersion: chaos-mesh.org/v1alpha1
kind: Workflow
metadata:
  name: payment-resilience-test
spec:
  entry: main
  templates:
  - name: main
    templateType: Serial
    children:
    - baseline-check      # 验证稳态
    - inject-pod-kill     # 注入 Pod 故障
    - observe-slo         # 观察 SLO
    - cleanup             # 清理

  - name: baseline-check
    templateType: Task
    task:
      container:
        image: slo-checker
        command: ["check-slo", "--target=payment", "--slo=p99<500ms"]

  - name: inject-pod-kill
    templateType: PodKill
    podKill:
      selector:
        namespaces: [production]
        labelSelectors:
          app: payment
      mode: fixed-percent
      value: "50"

  - name: observe-slo
    templateType: Task
    task:
      container:
        image: slo-checker
        command: ["check-slo", "--target=payment", "--duration=5m"]
```

### 发现五：Game Day 组织实践

| 阶段 | 活动 | 产出 |
|------|------|------|
| **准备** | 选择场景、设定爆炸半径、通知干系人 | 实验计划 |
| **执行** | 逐步注入故障、监控系统 | 实时观测数据 |
| **分析** | 评估系统表现、识别弱点 | 弱点清单 |
| **改进** | 修复发现的问题、更新 Runbook | Action Items |
| **复盘** | Blameless Postmortem | 经验文档 |

## 结论与建议

1. **混沌工程不是"破坏"，而是"验证"**：科学方法的核心是假设和验证。
2. **从非生产环境开始**：先在 Staging 环境验证实验设计和安全控制。
3. **SLO 是稳态的量化定义**：没有 SLO 的混沌实验等于盲目破坏。
4. **自动化是规模化关键**：手动实验无法覆盖所有场景，需要 CI/CD 集成。
5. **爆炸半径逐步扩大**：Pod → 节点 → AZ → Region，循序渐进。
6. **组织文化是最大障碍**：需要从"避免故障"转变为"主动发现弱点"。

## 参考资料

- Chaos Mesh: https://chaos-mesh.org/
- Litmus Chaos: https://litmuschaos.io/
- Principles of Chaos: https://principlesofchaos.org/
- [[可靠性/混沌工程/|混沌工程目录]]
- [[可靠性/index.md|可靠性目录]]
- [[研究/disaster-recovery-bcp.md|灾难恢复研究]]

## Related

- [[综合/slo-observability.md|SLO × 可观测性]]
- [[概念/chaos-engineering.md|混沌工程概念]]
