---
title: KUDIG Prompt 模板集：故障排查、架构评审、配置生成与学习路径
description: '## 故障排查 Prompt'
summary: '## 故障排查 Prompt'
category: reference
tags:
- k8s
- prompts
- troubleshooting
- architecture-review
- config-generator
- learning-path
- etcd
- apiserver
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Prompt 模板集：故障排查、架构评审、配置生成与学习路径 是什么
- 如何 KUDIG Prompt 模板集：故障排查、架构评审、配置生成与学习路径
trigger_keywords:
- KUDIG
- Prompt
- 模板集：故障排查
- 架构评审
- 配置生成与学习路径
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Prompt 模板集

## 故障排查 Prompt

核心流程：
1. **故障定位**：确认问题属于哪个知识域
2. **快速诊断**：按优先级执行检查
3. **深度诊断**：快速检查未定位时深入排查
4. **修复方案**：推荐操作 + 风险等级 + 回滚方案
5. **关联文档**：FTA 故障树、技能卡片、最佳实践

### 意图路由规则

| 用户查询关键词 | 路由目标 |
|----------------|----------|
| Pod 启动失败、CrashLoopBackOff、Pending | 工作负载 → 故障诊断/topic-fta/pod-fta |
| etcd、控制平面、apiserver | 集群基础 → 故障诊断/topic-fta/apiserver-fta |
| 网络不通、[[Service|Service]]、DNS | 网络 → 故障诊断/topic-fta/dns-fta |
| 存储、PV、PVC | 存储 → 故障诊断/topic-fta/csi-fta |
| 权限、RBAC、认证 | 安全 → 故障诊断/topic-fta/rbac-fta |

## 架构评审 Prompt

基于 KUDIG 最佳实践的架构评审清单，覆盖：
- 高可用设计、安全合规、可观测性
- 资源规划、扩缩容策略、灾备方案

## 配置生成 Prompt

根据场景自动生成 K8s YAML 配置：
- 输入：场景描述 + 约束条件
- 输出：符合最佳实践的 YAML 清单

## 学习路径 Prompt

根据工程师水平和目标定制学习计划：
- 输入：当前水平 + 目标岗位 + 可用时间
- 输出：分阶段学习路径 + 推荐文档列表

---

> 来源：prompts/*.md（共 4 篇）

## Prompt 使用指南

### Prompt 分类

| 类别 | 用途 | 示例场景 |
|---|---|---|
| 诊断类 | 故障排查 | "Pod CrashLoopBackOff 如何排查？" |
| 配置类 | 资源配置 | "如何配置 HPA 自动扩缩容？" |
| 优化类 | 性能优化 | "如何优化 API Server 性能？" |
| 学习类 | 概念理解 | "解释 Pod 和 Container 的关系" |

### Prompt 编写原则

1. **明确上下文**：提供 K8s 版本、环境信息
2. **具体描述**：包含错误信息、现象描述
3. **期望输出**：说明需要的答案形式
4. **约束条件**：标注风险等级、操作限制

### 高效 Prompt 模板

```
【环境】K8s 1.32 + Calico + 阿里云
【问题】Pod 状态 CrashLoopBackOff，重启 10 次
【日志】<粘贴关键日志>
【已尝试】kubectl logs 显示 OOM
【需求】排查步骤 + 修复方案 + 预防措施
```

## 面试要点

1. **Q：如何编写高效的 AI 辅助诊断 Prompt？**
   A：提供完整上下文、具体错误信息、已尝试步骤、明确期望输出。

2. **Q：AI 辅助运维的局限性？**
   A：无法访问实际环境、可能产生幻觉、需要人工验证、敏感信息保护。

3. **Q：如何建立 Prompt 知识库？**
   A：分类整理、模板化、定期更新、效果评估、团队共享。

## Related

- [[技能/skill-k8s-node-notready-SKILL.md|SKILL]].md|skill-k8s-node-notready-SKILL]] — Skill
- [[故障诊断/FTA故障树/list/pod-fta.md|pod-fta]] — pod-fta
- [[故障诊断/FTA故障树/list/dns-fta.md|dns-fta]] — dns-fta
- [[etcd]] — etcd
- [[apiserver-fta]] — API Server 异常故障树分析
- observability/07-tools/26-troubleshooting-tools|100 - 故障排查增强工具]] — Cross-reference
- [[可观测性/总览/25-troubleshooting-overview.md|10 - Kubernetes 生产环境故障排查全攻略 (Production Troubleshooting Guide)]] — Cross-reference
- [[技能/skill-assets-escalation-template.md|Escalation Template]] — Cross-reference
- [[集群基础/控制平面/06-plane-troubleshooting.md|控制平面故障排查手册 (Control Plane Troubleshooting Handbook)]] — Cross-reference
- [[平台工程/运维/15-production-troubleshooting.md|生产环境故障诊断 (Production Troubleshooting)]] — Cross-reference


<!-- risk-assessed -->
