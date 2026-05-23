---
title: KUDIG Prompt 模板集：故障排查、架构评审、配置生成与学习路径
description: '## 故障排查 Prompt'
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
created: "2026-05-23"
---

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
| Pod 启动失败、CrashLoopBackOff、Pending | domain-02-workloads-applications → domain-10-troubleshooting-diagnostics/topic-fta/pod-fta |
| etcd、控制平面、apiserver | domain-01-cluster-fundamentals → domain-10-troubleshooting-diagnostics/topic-fta/apiserver-fta |
| 网络不通、[[Service|Service]]、DNS | domain-03-networking-traffic → domain-10-troubleshooting-diagnostics/topic-fta/dns-fta |
| 存储、PV、PVC | domain-04-storage-data → domain-10-troubleshooting-diagnostics/topic-fta/csi-fta |
| 权限、RBAC、认证 | domain-05-security-compliance → domain-10-troubleshooting-diagnostics/topic-fta/rbac-fta |

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

## Related

- [[skills/skill-k8s-node-notready-SKILL|SKILL]].md|skill-k8s-node-notready-SKILL]] — Skill
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta|pod-fta]] — pod-fta
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta|dns-fta]] — dns-fta
- [[etcd]] — etcd
- [[apiserver-fta]] — API Server 异常故障树分析
- observability/07-tools/26-troubleshooting-tools|100 - 故障排查增强工具]] — Cross-reference
- [[domain-06-observability/01-overview/25-troubleshooting-overview|10 - Kubernetes 生产环境故障排查全攻略 (Production Troubleshooting Guide)]] — Cross-reference
- [[skills/skill-assets-escalation-template|Escalation Template]] — Cross-reference
- [[domain-01-cluster-fundamentals/03-control-plane/06-plane-troubleshooting|控制平面故障排查手册 (Control Plane Troubleshooting Handbook)]] — Cross-reference
- [[domain-07-platform-engineering/operate/15-production-troubleshooting|生产环境故障诊断 (Production Troubleshooting)]] — Cross-reference
