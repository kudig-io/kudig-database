---
title: topic-skills MOC (skills)
description: '### 专题定位'
summary: '### 专题定位'
category: skills
tags:
- k8s
- troubleshooting
- skill
- etcd
- argocd
- rbac
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
- topic-skills MOC 是什么
- 如何 topic-skills MOC
trigger_keywords:
- topic-skills
- MOC
prerequisites:
- kubectl-basics
- gitops-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-skills MOC

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-skills |
| **文档数量** | 32 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

### 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[19-故障诊断/08-技能体系/01-node-notready.md|[[节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation|节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation]]iagnosis & Remediation — 数字人播报脚本|Node NotReady]] Diagnosis & Remediation]] |  | [[SKILL|skill]], daily-ops |  |
| 2 | [[19-故障诊断/08-技能体系/02-pod-crashloop-oomkilled.md|[[Pod CrashLoopBackOff & OOMKilled 诊断与修复|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]]] |  | skill, daily-ops |  |
| 3 | [[19-故障诊断/08-技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]] |  | skill, daily-ops |  |
| 4 | [[19-故障诊断/08-技能体系/04-dns-resolution-failure.md|DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation]] |  | skill, daily-ops |  |
| 5 | [[19-故障诊断/08-技能体系/05-service-connectivity.md|Service 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis]] |  | skill, daily-ops |  |
| 6 | [[19-故障诊断/08-技能体系/06-certificate-expiry.md|证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis]] |  | skill, daily-ops |  |
| 7 | [[19-故障诊断/08-技能体系/07-pvc-storage-failure.md|PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation]] |  | skill, daily-ops, storage |  |
| 8 | [[19-故障诊断/08-技能体系/08-deployment-rollout-failure.md|Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis]] |  | skill, daily-ops, deployment |  |
| 9 | [[19-故障诊断/08-技能体系/09-rbac-quota-failure.md|RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting]] |  | skill, daily-ops, rbac |  |
| 10 | [[19-故障诊断/08-技能体系/10-image-pull-failure.md|镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting]] |  | skill, daily-ops |  |
| 11 | [[19-故障诊断/08-技能体系/11-control-plane-failure.md|11-control-plane-failure]]
- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 技能体系架构

### 技能分层模型

```
L4 专家技能
├── 架构设计与容灾规划
├── AI Agent 诊断引擎
└── 混沌工程与韧性测试

L3 高级技能
├── FTA 故障树分析 (33 个)
├── Operator 开发
└── 性能调优与容量规划

L2 进阶技能
├── 网络诊断 (CNI/Service/Ingress)
├── 存储管理 (CSI/PV/PVC)
├── 安全审计 (RBAC/PSA/NetworkPolicy)
└── 可观测性 (监控/日志/追踪)

L1 基础技能
├── kubectl 操作
├── Pod 故障排查
├── 节点检查与维护
└── 日志查看与分析
```

### 技能依赖关系

| 技能 | 前置技能 | 关联 FTA |
|------|----------|----------|
| Pod 排查 | kubectl 基础 | pod-fta |
| 节点排查 | Pod 排查 + Linux | node-fta |
| 网络诊断 | Pod 排查 + 网络基础 | service-fta, dns-fta |
| 存储管理 | Pod 排查 + 存储基础 | csi-fta |
| HPA 调优 | 监控基础 | hpa-fta, vpa-fta |

### 培训建议

1. **新人入职**: L1 基础技能 → 2 周内掌握 kubectl 和 Pod 排查
2. **初级 SRE**: L2 进阶技能 → 1-3 月掌握网络/存储/安全
3. **高级 SRE**: L3 FTA 故障树 → 3-6 月掌握结构化诊断
4. **架构师**: L4 专家技能 → 持续学习和实践

## Related

- [[gitops-argocd-fta]] — GitOps(ArgoCD) 异常故障树分析
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
