---
title: 故障诊断 MOC
description: 故障诊断 知识域导航页，覆盖 48 篇文档
summary: 故障诊断 知识域导航页，覆盖 48 篇文档
category: moc
tags:
- k8s
- moc
- k8s
- etcd
- apiserver
- helm
- argocd
- hpa
- vpa
- statefulset
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 故障诊断 MOC 是什么
- 如何 故障诊断 MOC
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 故障诊断 MOC 故障排查
- 故障诊断 MOC 排障步骤
trigger_keywords:
- 故障诊断
- MOC
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- gitops-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 故障诊断 MOC

> **MOC 版本**: 1.0
> **知识域**: 故障诊断
> **文档数量**: 48 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

故障排查 — 通用方法论、常见故障模式、诊断工具链

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | 故障诊断 |
| **文档数量** | 48 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | index.md|Domain-12 故障排查 — 开源项目索引]] |  | k8s, troubleshooting, guide |  |
| 2 | [[故障诊断/00-core-troubleshooting/01-control-plane-apiserver-troubleshooting.md|01 - [[API Server 故障排查|API Server 故障排查]] (API Server Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 3 | [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|02 - etcd 故障排查 (etcd Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 4 | [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting.md|03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 5 | [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|04 - CSI 存储驱动故障排查 (CSI Driver Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 6 | [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|05 - Pod Pending 状态深度诊断 (Pod Pending Diagnosis)]] |  | k8s, troubleshooting, guide |  |
| 7 | [[19-故障诊断/01-核心排障/06-node-notready-diagnosis.md|06 - Node NotReady 状态深度诊断 (Node NotReady Diagnosis)]] |  | k8s, troubleshooting, guide |  |
| 8 | [[19-故障诊断/01-核心排障/07-oom-memory-diagnosis.md|07 - OOM和内存问题诊断]] |  | k8s, troubleshooting, guide |  |
| 9 | [[19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting.md|08 - Pod 全面故障排查 (Pod Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 10 | [[19-故障诊断/02-资源排障/09-node-comprehensive-troubleshooting.md|09 - Node 全面故障排查 (Node Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 11 | [[19-故障诊断/02-资源排障/10-service-comprehensive-troubleshooting.md|10 - Service 全面故障排查 (Service Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 12 | [[19-故障诊断/02-资源排障/11-deployment-comprehensive-troubleshooting.md|11 - Deployment 全面故障排查 (Deployment Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 13 | [[19-故障诊断/02-资源排障/12-rbac-quota-troubleshooting.md|12 - RBAC与ResourceQuota 故障排查 (RBAC & Quota Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 14 | [[19-故障诊断/02-资源排障/13-certificate-troubleshooting.md|13 - 证书故障排查 (Certificate Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 15 | [[19-故障诊断/02-资源排障/14-pvc-storage-troubleshooting.md|14 - PVC与存储全面故障排查 (PVC & Storage Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 16 | [[19-故障诊断/02-资源排障/15-ingress-troubleshooting.md|15 - Ingress 故障排查 (Ingress Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 17 | [[19-故障诊断/02-资源排障/16-networkpolicy-troubleshooting.md|16 - NetworkPolicy 故障排查 (NetworkPolicy Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 18 | [[19-故障诊断/02-资源排障/17-hpa-vpa-troubleshooting.md|17 - HPA/VPA 故障排查 (HPA/VPA Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 19 | [[19-故障诊断/02-资源排障/18-cronjob-troubleshooting.md|18 - CronJob 故障排查 (CronJob Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 20 | [[19-故障诊断/02-资源排障/19-configmap-secret-troubleshooting.md|19 - ConfigMap/Secret 故障排查 (ConfigMap/Secret Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 21 | [[19-故障诊断/02-资源排障/20-daemonset-troubleshooting.md|20 - DaemonSet 故障排查 (DaemonSet Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 22 | [[19-故障诊断/02-资源排障/21-statefulset-troubleshooting.md|21 - StatefulSet 故障排查 (StatefulSet Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 23 | [[19-故障诊断/02-资源排障/22-job-troubleshooting.md|22 - Job 故障排查 (Job Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 24 | [[19-故障诊断/02-资源排障/23-namespace-troubleshooting.md|23 - Namespace 故障排查 (Namespace Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 25 | [[19-故障诊断/02-资源排障/24-quota-limitrange-troubleshooting.md|24 - Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 26 | [[19-故障诊断/03-基础设施排障/25-network-connectivity-troubleshooting.md|25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 27 | [[19-故障诊断/03-基础设施排障/26-dns-troubleshooting.md|26 - DNS 故障排查 (DNS Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 28 | [[19-故障诊断/03-基础设施排障/27-image-registry-troubleshooting.md|27 - 镜像仓库故障排查 (Image Registry Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 29 | [[19-故障诊断/03-基础设施排障/28-cluster-autoscaler-troubleshooting.md|28 - 集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 30 | [[19-故障诊断/03-基础设施排障/29-cloud-provider-troubleshooting.md|29 - 云提供商集成故障排查 (Cloud Provider Integration Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 31 | [[19-故障诊断/03-基础设施排障/30-monitoring-alerting-troubleshooting.md|30 - 监控告警故障排查 (Monitoring and Alerting Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 32 | [[19-故障诊断/03-基础设施排障/31-backup-restore-troubleshooting.md|31 - 备份恢复故障排查 (Backup and Restore Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 33 | [[19-故障诊断/03-基础设施排障/32-security-troubleshooting.md|32 - 安全相关故障排查 (Security Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 34 | [[19-故障诊断/03-基础设施排障/33-performance-bottleneck-troubleshooting.md|33 - 性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 35 | [[19-故障诊断/03-基础设施排障/34-upgrade-migration-troubleshooting.md|34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 36 | [[19-故障诊断/04-高级排障/35-node-component-troubleshooting.md|35 - 节点组件故障排查 (Node Component Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 37 | [[19-故障诊断/04-高级排障/36-helm-chart-troubleshooting.md|36 - Helm Chart 故障排查 (Helm Chart Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 38 | [[19-故障诊断/04-高级排障/37-multi-cluster-management-troubleshooting.md|37 - 多集群管理故障排查 (Multi-Cluster Management Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 39 | [[19-故障诊断/04-高级排障/38-gitops-argocd-troubleshooting.md|38 - GitOps和ArgoCD故障排查 (GitOps and ArgoCD Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 40 | [[19-故障诊断/04-高级排障/39-enterprise-monitoring-alerting-system.md|39 - 企业级监控告警体系 (Enterprise Monitoring and Alerting System)]] |  | k8s, troubleshooting, guide |  |
| 41 | [[19-故障诊断/04-高级排障/40-large-scale-cluster-operations.md|40 - 大规模集群运维 (Large Scale Cluster Operations)]] |  | k8s, troubleshooting, guide |  |
| 42 | [[19-故障诊断/04-高级排障/41-event-driven-architecture-troubleshooting.md|41 - 事件驱动架构故障排查 (Event-Driven Architecture Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 43 | [[19-故障诊断/04-高级排障/42-chaos-engineering-fault-injection-testing.md|42 - 混沌工程和故障注入测试 (Chaos Engineering and Fault Injection Testing)]] |  | k8s, troubleshooting, guide |  |
| 44 | [[19-故障诊断/04-高级排障/43-symptom-sop-mapping.md|症状 → SOP 映射手册]] |  | k8s, troubleshooting, guide |  |
| 45 | [[19-故障诊断/04-高级排障/44-kind-k3s-single-node-troubleshooting.md|Kind / K3s 单机集群故障排查]] |  | k8s, troubleshooting, guide |  |
| 46 | [[19-故障诊断/05-JVM调优/99-java-performance-resource-sizing-guide.md|Java 应用性能调优与资源 Sizing 指南]] |  | k8s, troubleshooting, guide |  |
| 47 | [[19-故障诊断/05-JVM调优/99-jvm-gc-container-tuning-guide.md|JVM GC 容器调优深度指南]] |  | k8s, troubleshooting, guide |  |
| 48 | [[19-故障诊断/SUMMARY.md|Domain-12 故障排查文档体系完整总结报告]] |  | k8s, troubleshooting, guide |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph 故障诊断
        A["Domain-12 故障排查 — 开源项目索引"]
    B["01 - API Server 故障排查 (API Server Troubleshooting)"]
    C["02 - etcd 故障排查 (etcd Troubleshooting)"]
    D["03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)"]
    E["04 - CSI 存储驱动故障排查 (CSI Driver Troubleshooting)"]
    F["05 - Pod Pending 状态深度诊断 (Pod Pending Diagnosis)"]
    end

    A --> B
    A --> C
    A --> D
    A --> E
    A --> F

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
```

---

## 关联入口

| 入口 | 说明 |
|---|---|
| FTA 故障树 | 故障诊断 相关故障树分析 |
| Skills 技能 | 故障诊断 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 48 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*


<!-- risk-assessed -->
