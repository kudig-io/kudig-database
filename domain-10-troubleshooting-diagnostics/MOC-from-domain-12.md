---
title: domain-10-troubleshooting-diagnostics MOC
description: domain-10-troubleshooting-diagnostics 知识域导航页，覆盖 48 篇文档
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
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- domain-10-troubleshooting-diagnostics MOC 是什么
- 如何 domain-10-troubleshooting-diagnostics MOC
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- domain-10-troubleshooting-diagnostics MOC 故障排查
- domain-10-troubleshooting-diagnostics MOC 排障步骤
trigger_keywords:
- domain-10-troubleshooting-diagnostics
- MOC
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- gitops-basics
- etcd-basics
created: "2026-05-23"
---

# domain-10-troubleshooting-diagnostics MOC

> **MOC 版本**: 1.0
> **知识域**: domain-10-troubleshooting-diagnostics
> **文档数量**: 48 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

故障排查 — 通用方法论、常见故障模式、诊断工具链

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | domain-10-troubleshooting-diagnostics |
| **文档数量** | 48 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | index.md|Domain-12 故障排查 — 开源项目索引]] |  | k8s, troubleshooting, guide |  |
| 2 | [[domain-10-troubleshooting-diagnostics/01-control-plane-apiserver-troubleshooting|01 - [[API Server 故障排查|API Server 故障排查]] (API Server Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 3 | [[domain-10-troubleshooting-diagnostics/02-control-plane-etcd-troubleshooting|02 - etcd 故障排查 (etcd Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 4 | [[domain-10-troubleshooting-diagnostics/03-networking-cni-troubleshooting|03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 5 | [[domain-10-troubleshooting-diagnostics/04-storage-csi-troubleshooting|04 - CSI 存储驱动故障排查 (CSI Driver Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 6 | [[domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis|05 - Pod Pending 状态深度诊断 (Pod Pending Diagnosis)]] |  | k8s, troubleshooting, guide |  |
| 7 | [[domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis|06 - Node NotReady 状态深度诊断 (Node NotReady Diagnosis)]] |  | k8s, troubleshooting, guide |  |
| 8 | [[domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis|07 - OOM和内存问题诊断]] |  | k8s, troubleshooting, guide |  |
| 9 | [[domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting|08 - Pod 全面故障排查 (Pod Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 10 | [[domain-10-troubleshooting-diagnostics/09-node-comprehensive-troubleshooting|09 - Node 全面故障排查 (Node Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 11 | [[domain-10-troubleshooting-diagnostics/10-service-comprehensive-troubleshooting|10 - Service 全面故障排查 (Service Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 12 | [[domain-10-troubleshooting-diagnostics/11-deployment-comprehensive-troubleshooting|11 - Deployment 全面故障排查 (Deployment Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 13 | [[domain-10-troubleshooting-diagnostics/12-rbac-quota-troubleshooting|12 - RBAC与ResourceQuota 故障排查 (RBAC & Quota Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 14 | [[domain-10-troubleshooting-diagnostics/13-certificate-troubleshooting|13 - 证书故障排查 (Certificate Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 15 | [[domain-10-troubleshooting-diagnostics/14-pvc-storage-troubleshooting|14 - PVC与存储全面故障排查 (PVC & Storage Comprehensive Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 16 | [[domain-10-troubleshooting-diagnostics/15-ingress-troubleshooting|15 - Ingress 故障排查 (Ingress Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 17 | [[domain-10-troubleshooting-diagnostics/16-networkpolicy-troubleshooting|16 - NetworkPolicy 故障排查 (NetworkPolicy Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 18 | [[domain-10-troubleshooting-diagnostics/17-hpa-vpa-troubleshooting|17 - HPA/VPA 故障排查 (HPA/VPA Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 19 | [[domain-10-troubleshooting-diagnostics/18-cronjob-troubleshooting|18 - CronJob 故障排查 (CronJob Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 20 | [[domain-10-troubleshooting-diagnostics/19-configmap-secret-troubleshooting|19 - ConfigMap/Secret 故障排查 (ConfigMap/Secret Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 21 | [[domain-10-troubleshooting-diagnostics/20-daemonset-troubleshooting|20 - DaemonSet 故障排查 (DaemonSet Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 22 | [[domain-10-troubleshooting-diagnostics/21-statefulset-troubleshooting|21 - StatefulSet 故障排查 (StatefulSet Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 23 | [[domain-10-troubleshooting-diagnostics/22-job-troubleshooting|22 - Job 故障排查 (Job Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 24 | [[domain-10-troubleshooting-diagnostics/23-namespace-troubleshooting|23 - Namespace 故障排查 (Namespace Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 25 | [[domain-10-troubleshooting-diagnostics/24-quota-limitrange-troubleshooting|24 - Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 26 | [[domain-10-troubleshooting-diagnostics/25-network-connectivity-troubleshooting|25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 27 | [[domain-10-troubleshooting-diagnostics/26-dns-troubleshooting|26 - DNS 故障排查 (DNS Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 28 | [[domain-10-troubleshooting-diagnostics/27-image-registry-troubleshooting|27 - 镜像仓库故障排查 (Image Registry Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 29 | [[domain-10-troubleshooting-diagnostics/28-cluster-autoscaler-troubleshooting|28 - 集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 30 | [[domain-10-troubleshooting-diagnostics/29-cloud-provider-troubleshooting|29 - 云提供商集成故障排查 (Cloud Provider Integration Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 31 | [[domain-10-troubleshooting-diagnostics/30-monitoring-alerting-troubleshooting|30 - 监控告警故障排查 (Monitoring and Alerting Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 32 | [[domain-10-troubleshooting-diagnostics/31-backup-restore-troubleshooting|31 - 备份恢复故障排查 (Backup and Restore Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 33 | [[domain-10-troubleshooting-diagnostics/32-security-troubleshooting|32 - 安全相关故障排查 (Security Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 34 | [[domain-10-troubleshooting-diagnostics/33-performance-bottleneck-troubleshooting|33 - 性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 35 | [[domain-10-troubleshooting-diagnostics/34-upgrade-migration-troubleshooting|34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 36 | [[domain-10-troubleshooting-diagnostics/35-node-component-troubleshooting|35 - 节点组件故障排查 (Node Component Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 37 | [[domain-10-troubleshooting-diagnostics/36-helm-chart-troubleshooting|36 - Helm Chart 故障排查 (Helm Chart Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 38 | [[domain-10-troubleshooting-diagnostics/37-multi-cluster-management-troubleshooting|37 - 多集群管理故障排查 (Multi-Cluster Management Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 39 | [[domain-10-troubleshooting-diagnostics/38-gitops-argocd-troubleshooting|38 - GitOps和ArgoCD故障排查 (GitOps and ArgoCD Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 40 | [[domain-10-troubleshooting-diagnostics/39-enterprise-monitoring-alerting-system|39 - 企业级监控告警体系 (Enterprise Monitoring and Alerting System)]] |  | k8s, troubleshooting, guide |  |
| 41 | [[domain-10-troubleshooting-diagnostics/40-large-scale-cluster-operations|40 - 大规模集群运维 (Large Scale Cluster Operations)]] |  | k8s, troubleshooting, guide |  |
| 42 | [[domain-10-troubleshooting-diagnostics/41-event-driven-architecture-troubleshooting|41 - 事件驱动架构故障排查 (Event-Driven Architecture Troubleshooting)]] |  | k8s, troubleshooting, guide |  |
| 43 | [[domain-10-troubleshooting-diagnostics/42-chaos-engineering-fault-injection-testing|42 - 混沌工程和故障注入测试 (Chaos Engineering and Fault Injection Testing)]] |  | k8s, troubleshooting, guide |  |
| 44 | [[domain-10-troubleshooting-diagnostics/43-symptom-sop-mapping|症状 → SOP 映射手册]] |  | k8s, troubleshooting, guide |  |
| 45 | [[domain-10-troubleshooting-diagnostics/44-kind-k3s-single-node-troubleshooting|Kind / K3s 单机集群故障排查]] |  | k8s, troubleshooting, guide |  |
| 46 | [[domain-10-troubleshooting-diagnostics/99-java-performance-resource-sizing-guide|Java 应用性能调优与资源 Sizing 指南]] |  | k8s, troubleshooting, guide |  |
| 47 | [[domain-10-troubleshooting-diagnostics/99-jvm-gc-container-tuning-guide|JVM GC 容器调优深度指南]] |  | k8s, troubleshooting, guide |  |
| 48 | [[domain-10-troubleshooting-diagnostics/SUMMARY|Domain-12 故障排查文档体系完整总结报告]] |  | k8s, troubleshooting, guide |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph domain-10-troubleshooting-diagnostics
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
| FTA 故障树 | domain-10-troubleshooting-diagnostics 相关故障树分析 |
| Skills 技能 | domain-10-troubleshooting-diagnostics 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 48 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*
