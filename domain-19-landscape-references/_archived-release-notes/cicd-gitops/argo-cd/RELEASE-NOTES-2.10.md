---
title: argo-cd v2.10 Release Notes
description: argo-cd v2.10 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v2.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v2.10 Release Notes 是什么
- 如何 argo-cd v2.10 Release Notes
trigger_keywords:
- argo-cd
- v2.10
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




# argo-cd v2.10 Release Notes

Source: [v2.10.20](https://github.com/argoproj/argo-cd/releases/tag/v2.10.20)

## Quick Start

### Non-HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.10.20/manifests/install.yaml
```
### HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.10.20/manifests/ha/install.yaml
```
## Release Signatures and Provenance

All [[Argo|Argo]] CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Dependency updates
* 7bd0c3669ff2fbb54678e4577f37941624e16b41: chore(deps): bump slsa-framework/slsa-github-generator from 2.0.0 to 2.1.0 (#23166) (#24467) (@alexmt)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.10.19...v2.10.20

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>



<!-- risk-assessed -->
