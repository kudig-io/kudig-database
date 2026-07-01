---
title: "Deployment × Service"
category: "synthesis"
tags: ["cross-domain", "workloads", "networking"]
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
relationships:
  - target: "[[entities/helm.md]]"
    type: uses
  - target: "[[entities/prometheus.md]]"
    type: uses
  - target: "[[entities/argocd.md]]"
    type: related_to
---

# Deployment × Service

## 概述
Deployment与Service是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。

## 关联场景

### 场景1：Deployment影响Service
当Deployment出现异常时，可能会直接影响Service的正常工作。例如：
- Deployment配置错误导致Service无法正确识别资源
- Deployment性能瓶颈导致Service响应延迟

### 场景2：Service反向影响Deployment
Service的配置和状态也会反过来影响Deployment：
- Service资源不足导致Deployment无法完成预定操作
- Service网络隔离导致Deployment通信失败

## 最佳实践
1. 在配置Deployment时充分考虑Service的约束和限制
2. 建立Deployment和Service的联合监控和告警
3. 制定涉及Deployment和Service的变更管理流程
4. 定期进行涉及Deployment和Service的混沌工程演练

## 常见问题

### 问题1：Deployment和Service配置冲突
**症状**：服务行为异常，配置未按预期生效
**根因**：Deployment和Service的配置存在隐式冲突
**修复**：统一配置管理，使用GitOps追踪配置变更

### 问题2：Deployment变更导致Service中断
**症状**：Deployment变更后Service服务不可用
**根因**：变更影响评估未覆盖Service维度
**修复**：建立变更影响矩阵，强制进行跨域影响评估

## 工具推荐
- kubectl：基础诊断
- [[entities/helm.md|Helm]]/Kustomize：配置管理
- [[entities/prometheus.md|Prometheus]]/Grafana：联合监控
- [[entities/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[Deployment]]
- [[Service]]
## Related

- [[entities/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[entities/argo.md|Argo Workflows]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[concepts/etcd-×-PVC.md|etcd-×-PVC]]
- [[concepts/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[concepts/apiserver-×-Service.md|apiserver-×-Service]]
- [[concepts/StatefulSet-×-Service.md|StatefulSet-×-Service]]
