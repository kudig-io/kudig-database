---
title: argo-cd v1.5 Release Notes
description: argo-cd v1.5 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v1.5 Release Notes 是什么
- 如何 argo-cd v1.5 Release Notes
trigger_keywords:
- argo-cd
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# argo-cd v1.5 Release Notes

Source: v1.5.8](https://github.com/argoproj/argo-cd/releases/tag/v1.5.8)

## Quick Start
### Non-HA:
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.5.8/manifests/install.yaml
```
### HA:
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.5.8/manifests/ha/install.yaml
```
### Bug Fixes

* fix: upgrade awscli version (#3774)
* fix: html encode login error/description before rendering it (#3773)
* fix: oidc should set samesite cookie (#3632)
* fix: avoid panic in badge handler (#3741)

<!-- risk-assessed -->
