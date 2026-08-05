---
title: topic-skills MOC
description: topic-skills 专题导航页，覆盖 32 篇文档
summary: topic-skills 专题导航页，覆盖 32 篇文档
category: moc
tags:
- k8s
- moc
- skill
- etcd
- hpa
- vpa
- ingress
- gateway
- rbac
- rag
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- topic-skills MOC 是什么
- 如何 topic-skills MOC
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- topic-skills MOC 故障排查
- topic-skills MOC 排障步骤
trigger_keywords:
- topic-skills
- MOC
- troubleshooting
- diagnostics
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
- gpu-scheduling-basics
skill_id: SKILL-MOC-001
skill_name: topic-skills MOC
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-skills MOC

> **MOC 版本**: 1.0
> **专题**: topic-skills
> **文档数量**: 32 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

操作技能 — 场景化运维操作卡片

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-skills |
| **文档数量** | 32 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[19-故障诊断/08-技能体系/01-node-notready.md|[[19-故障诊断/08-技能体系/01-node-notready\|节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation]]iagnosis & Remediation — 数字人播报脚本|Node NotReady]] Diagnosis & Remediation]] |  | [[SKILL|skill]], daily-ops |  |
| 2 | [[19-故障诊断/08-技能体系/02-pod-crashloop-oomkilled.md|[[19-故障诊断/08-技能体系/02-pod-crashloop-oomkilled|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]]] |  | skill, daily-ops |  |
| 3 | [[19-故障诊断/08-技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]] |  | skill, daily-ops |  |
| 4 | [[19-故障诊断/08-技能体系/04-dns-resolution-failure.md|DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation]] |  | skill, daily-ops |  |
| 5 | [[19-故障诊断/08-技能体系/05-service-connectivity.md|Service 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis]] |  | skill, daily-ops |  |
| 6 | [[19-故障诊断/08-技能体系/06-certificate-expiry.md|证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis]] |  | skill, daily-ops |  |
| 7 | [[19-故障诊断/08-技能体系/08-pvc-storage-failure.md|PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation]] |  | skill, daily-ops, storage |  |
| 8 | [[19-故障诊断/08-技能体系/09-deployment-rollout-failure.md|Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis]] |  | skill, daily-ops, deployment |  |
| 9 | [[19-故障诊断/08-技能体系/10-rbac-quota-failure.md|RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting]] |  | skill, daily-ops, rbac |  |
| 10 | [[19-故障诊断/08-技能体系/11-image-pull-failure.md|镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting]] |  | skill, daily-ops |  |
| 11 | [[19-故障诊断/08-技能体系/12-control-plane-failure.md|etcd 与控制平面故障诊断与修复 / etcd & Control Plane Failure Diagnosis & Remediation]] |  | skill, daily-ops |  |
| 12 | [[19-故障诊断/08-技能体系/13-autoscaling-failure.md|HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation]] |  | skill, daily-ops |  |
| 13 | [[19-故障诊断/08-技能体系/14-ingress-gateway-failure.md|Ingress/Gateway 路由故障诊断与修复 / Ingress & Gateway Routing Failure Diagnosis & Remediation]] |  | skill, daily-ops |  |
| 14 | [[19-故障诊断/08-技能体系/15-configmap-secret-failure.md|ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting]] |  | skill, daily-ops, configuration |  |
| 15 | [[19-故障诊断/08-技能体系/16-monitoring-alerting-failure.md|监控告警体系故障诊断与修复 / Monitoring & Alerting System Diagnosis & Remediation]] |  | skill, daily-ops, monitoring |  |
| 16 | [[19-故障诊断/08-技能体系/17-logging-pipeline-failure.md|日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation]] |  | skill, daily-ops |  |
| 17 | [[19-故障诊断/08-技能体系/18-performance-bottleneck.md|性能瓶颈诊断与调优 / Performance Bottleneck Diagnosis & Tuning]] |  | skill, daily-ops, performance |  |
| 18 | [[19-故障诊断/08-技能体系/19-security-incident-response.md|安全事件应急响应 / Security Incident Response]] |  | skill, daily-ops, security |  |
| 19 | [[19-故障诊断/08-技能体系/21-skill-local-demo-guide.md|Skill 本地运行 Demo 指南]] |  | skill, daily-ops, guide |  |
| 20 | [[19-故障诊断/08-技能体系/ENHANCEMENT-RECORD.md|topic-skills 全面增强记录]] |  | skill, daily-ops |  |
| 21 | [[19-故障诊断/08-技能体系/assessment/answer-keys/k8s-fundamentals-quiz-answers.md|K8s 基础知识考核 - 答案解析]] |  | skill, daily-ops |  |
| 22 | [[19-故障诊断/08-技能体系/assessment/daily-check-quiz.md|每日一题]] |  | skill, daily-ops |  |
| 23 | [[19-故障诊断/08-技能体系/assessment/k8s-fundamentals-quiz.md|K8s 基础知识考核]] |  | skill, daily-ops |  |
| 24 | [[19-故障诊断/08-技能体系/assessment/troubleshooting-lab-exam.md|故障排查实验考核]] |  | skill, daily-ops, troubleshooting |  |
| 25 | [[19-故障诊断/08-技能体系/skill-schema.md|Skill Schema (历史参考)]] |  | skill, daily-ops |  |
| 26 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/SKILL.md|K8s Node NotReady 诊断与修复]] |  | skill, daily-ops |  |
| 27 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/USAGE-GUIDE.md|Skills + FTA 使用指南 — k8s-node-notready & node-fta]] |  | skill, daily-ops, guide |  |
| 28 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/assets/escalation-template.md|升级消息模板 / Escalation Message Template]] |  | skill, daily-ops |  |
| 29 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/diagnostic-workflow.md|诊断工作流 / Diagnostic Workflow]] |  | skill, daily-ops |  |
| 30 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/remediation-playbook.md|修复操作手册 / Remediation Playbook]] |  | skill, daily-ops |  |
| 31 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/root-cause-catalog.md|根因分类 / Root Cause Catalog]] |  | skill, daily-ops |  |
| 32 | [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/version-matrix.md|版本兼容矩阵与知识进化 / Version Matrix & Knowledge Evolution]] |  | skill, daily-ops |  |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## Related

- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 网络 MOC — Cross-reference
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[01-集群基础/05-kubectl/02-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[01-集群基础/01-架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[06-存储/01-K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[06-存储/01-K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference
- [[21-生态参考/03-领域索引/MOC.md|topic-index MOC]] — Cross-reference

## 技能分级体系

| 级别 | 定义 | 典型技能 | 考核方式 |
|------|------|----------|----------|
| L1 | 基础操作，可按 SOP 执行 | Pod 日志查看、基本 describe | 实操考核 |
| L2 | 独立诊断，能定位常见根因 | DNS 排障、HPA 修复、证书续期 | 场景模拟 |
| L3 | 复杂诊断，能处理多故障并发 | 级联故障、控制平面异常 | 生产实战 |
| L4 | 专家级，能设计诊断体系和工具 | FEBM 体系建设、自动化诊断平台 | 架构评审 |

## 技能依赖关系

```
kubectl-basics (L1)
├─ pod-troubleshooting (L1)
│   ├─ crashloop-diagnosis (L2)
│   └─ oom-analysis (L2)
├─ networking-basics (L1)
│   ├─ dns-failure (L2)
│   ├─ service-connectivity (L2)
│   └─ network-policy (L2)
├─ scheduling-basics (L1)
│   ├─ hpa-diagnosis (L2)
│   └─ resource-management (L2)
└─ release-management (L1)
    ├─ canary-deployment (L2)
    ├─ blue-green-deployment (L2)
    └─ helm-chart-failure (L2)
```

## 培训建议

1. **新人入职**：先完成 L1 所有技能，熱练掌握 kubectl 基本操作
2. **月度演练**：每月模拟 1-2 个 L2 场景，保持手感
3. **事后复盘**：每次生产事故后提取新技能点，更新技能库
4. **交叉培训**：鼓励团队成员互相分享擅长领域的排障经验


<!-- risk-assessed -->
