---
title: KUDIG Prompts Catalog
description: '| 故障排查 Prompt | 系统化故障排查 | 用户报告故障现象时 |'
category: reference
tags:
- k8s
- prompts
- ai-agent
- troubleshooting
- intent-routing
- etcd
- apiserver
- hpa
- vpa
- statefulset
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Prompts Catalog 是什么
- 如何 KUDIG Prompts Catalog
trigger_keywords:
- KUDIG
- Prompts
- Catalog
prerequisites:
- kubectl-basics
- etcd-basics
---

# KUDIG Prompts Catalog

> KUDIG AI Prompt 模板集合，为 Agent 提供标准化的交互模式

---

## Prompt 模板列表

| Prompt | 用途 | 触发场景 |
|--------|------|---------|
| 故障排查 Prompt | 系统化故障排查 | 用户报告故障现象时 |
| 架构审查 Prompt | Kubernetes 架构评审 | 设计新集群或变更时 |
| 配置生成 Prompt | 生成 K8s YAML 配置 | 需要创建资源配置时 |
| 学习路径 Prompt | 个性化学习路径推荐 | 新手询问如何学习 K8s 时 |

---

## 故障排查 Prompt

核心排查流程，5 步标准化：

1. **故障定位** — 确认故障现象属于哪个知识域（控制平面/工作负载/网络/存储/安全）
2. **快速诊断** — 按优先级执行快速检查
3. **深度诊断** — 快速检查未定位时执行深入检查
4. **修复方案** — 推荐操作、风险等级、回滚方案
5. **关联文档** — FTA 故障树、技能卡片、最佳实践链接

### 意图路由规则

| 用户查询关键词 | 路由目标 |
|---|---|
| "Pod 启动失败", "CrashLoopBackOff", "Pending" | domain-02-workloads-applications → domain-10-troubleshooting-diagnostics/topic-fta/pod-fta |
| "etcd", "控制平面", "apiserver" | domain-01-cluster-fundamentals → domain-10-troubleshooting-diagnostics/topic-fta/apiserver-fta |
| "网络不通", "Service", "DNS" | domain-03-networking-traffic → domain-10-troubleshooting-diagnostics/topic-fta/dns-fta |
| "存储", "PV", "PVC" | domain-04-storage-data → domain-10-troubleshooting-diagnostics/topic-fta/csi-fta |
| "权限", "RBAC", "认证" | domain-05-security-compliance → domain-10-troubleshooting-diagnostics/topic-fta/rbac-fta |

---

## 架构审查 Prompt

用于审查 Kubernetes 集群架构设计，关注：
- 高可用性设计（控制平面副本、etcd 集群）
- 网络拓扑（CNI 选择、Service Mesh 需求）
- 存储策略（持久化存储方案、备份策略）
- 安全纵深（RBAC、NetworkPolicy、运行时安全）
- 可观测性（Metrics、Logs、Traces 集成）

---

## 配置生成 Prompt

用于生成标准化的 Kubernetes 资源配置：
- Deployment/StatefulSet/DaemonSet
- Service/Ingress/NetworkPolicy
- ConfigMap/Secret
- PVC/PV/StorageClass
- HPA/VPA

---

## 学习路径 Prompt

根据用户背景推荐学习路径：
- 零基础 → fundamentals → 系统培训 → On-Call 实战
- 有经验 → 查漏补缺 → 进阶内容 → 故障排查专项
- 阿里云 ACK 用户 → inner-training 路径

---

## 相关文档

- [[kudig-templates-catalog|文档模板目录]]
- [[skills/Agent Orchestration Patterns.md|Agent 编排模式]]
- [[references/KUDIG Templates and Agent Prompts.md|原版 Prompt 集合]]

## Related

- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[kudig-prompts-catalog]]
- [[domain-06-observability/07-tools/26-troubleshooting-tools|100 - 故障排查增强工具]] — Cross-reference
- [[domain-06-observability/01-overview/25-troubleshooting-overview|10 - Kubernetes 生产环境故障排查全攻略 (Production Troubleshooting Guide)]] — Cross-reference
- [[skills/skill-assets-escalation-template|Escalation Template]] — Cross-reference
- [[domain-01-cluster-fundamentals/03-control-plane/06-plane-troubleshooting|控制平面故障排查手册 (Control Plane Troubleshooting Handbook)]] — Cross-reference
- [[domain-07-platform-engineering/operate/15-production-troubleshooting|生产环境故障诊断 (Production Troubleshooting)]] — Cross-reference
