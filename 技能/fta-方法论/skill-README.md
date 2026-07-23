---
title: topic-skills — 工单智能体 Kubernetes 诊断 Skill 库 (skills)
description: '- 网络问题: [04-dns](./04-dns-resolution-failure.md) | [05-service](./05-service-connectivity.md)
  | [13-ingress](./13-ingress-gateway-failure.md)'
summary: '- 网络问题: [04-dns](./04-dns-resolution-failure.md) | [05-service](./05-service-connectivity.md)
  | [13-ingress](./13-ingress-gateway-failure.md)'
category: skills
tags:
- k8s
- troubleshooting
- skill
- job
- cronjob
- ingress
- gateway
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- topic-skills — 工单智能体 Kubernetes 诊断 Skill 库 是什么
- 如何 topic-skills — 工单智能体 Kubernetes 诊断 Skill 库
trigger_keywords:
- topic-skills
- 工单智能体
- Kubernetes
- 诊断
- Skill
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-skills — 工单智能体 Kubernetes 诊断 [[SKILL|Skill]] 库

### 故障诊断

- 节点问题: [01-node-notready](./01-node-notready.md) | [11-control-plane](./11-control-plane-failure.md)
- Pod 异常: [02-crashloop](./02-pod-crashloop-oomkilled.md) | [03-pending](./03-pod-pending.md) | [10-image-pull](./10-image-pull-failure.md)
- 网络问题: [04-dns](./04-dns-resolution-failure.md) | [05-[[Service|service]]](./05-service-connectivity.md) | [13-[[Ingress|ingress]]](./13-ingress-gateway-failure.md)
- 存储问题: [07-pvc-storage](./07-pvc-storage-failure.md)
- 配置问题: [14-configmap-secret](./14-configmap-secret-failure.md)

### 3. 症状 → Skill 快速查找



## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/diagnostic-overview/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]

## 技能文件结构规范

### FTA 故障树文件结构

每个 FTA 文件包含以下标准章节:

1. **诊断命令速查表** - 各节点的实际诊断命令
2. **故障树逻辑** - Mermaid 图或 JSON 结构
3. **生产案例** - 2 个真实案例(时间线+根因+修复)
4. **升级决策点** - P0/P1/P2 三级响应
5. **面试要点** - 3 个 Q&A
6. **相关链接** - 关联文档

### Skill 操作技能文件结构

1. **症状识别** - 症状模式表
2. **诊断工作流** - 分阶段检查步骤
3. **修复操作** - 按风险等级分类
4. **生产案例** - 真实案例
5. **升级决策点** - 响应级别
6. **面试要点** - Q&A

### 命令风险等级

| 等级 | 含义 | 示例 |
|------|------|------|
| 🟢 低风险 | 只读/信息收集 | kubectl get/describe/logs |
| 🟡 中风险 | 修改状态，通常可回滚 | kubectl scale/rollout restart |
| 🔴 高风险 | 可能造成数据丢失/服务中断 | kubectl delete --force |

## Related

- [[higress-fta]] — Higress 网关异常故障树分析
- [[cluster-upgrade-fta]] — 集群升级异常故障树分析
- [[job-cronjob-fta]] — Job/CronJob 异常故障树分析
- [[技能/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
