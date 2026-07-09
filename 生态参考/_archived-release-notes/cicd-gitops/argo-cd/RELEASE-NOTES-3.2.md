---
title: argo-cd v3.2 Release Notes
description: argo-cd v3.2 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v3.2 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v3.2 Release Notes 是什么
- 如何 argo-cd v3.2 Release Notes
trigger_keywords:
- argo-cd
- v3.2
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




# argo-cd v3.2 Release Notes

Source: [v3.2.8](https://github.com/argoproj/argo-cd/releases/tag/v3.2.8)

## Quick Start

### Non-HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.2.8/manifests/install.yaml
```
### HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.2.8/manifests/ha/install.yaml
```
## Release Signatures and Provenance

All [[Argo|Argo]] CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.

## Release Notes Blog Post
For a detailed breakdown of the key changes and improvements in this release, check out the [official blog post](https://blog.argoproj.io/argo-cd-v3-0-release-candidate-a0b933f4e58f)

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 65378e6d14dd94e2920ca5874eee8cef15dd3140: fix(UI): show RollingSync step clearly when labels match no step (cherry-pick #26877 for 3.2) (#26883) (@argo-cd-cherry-pick-bot[bot])
* 1515e91ce8d5f3b3cf38b179cb29fd878ef47726: fix: controller incorrectly detecting diff during app normalization (cherry-pick #27002 for 3.2) (#27012) (@argo-cd-cherry-pick-bot[bot])
* 5fca1ce7d8c42e5cf44053bf7eb3aea8ead73c79: fix: mitigation of grpc-go CVE-2026-33186 for release-3.2 (#26983) (@dudinea)
### Other work
* e7d33de05cb1f09f307dadc44fdc3295a1d05a1a: chore: use base ref for cherry-pick prs (cherry-pick #26551 for 3.2) (#26554) (@argo-cd-cherry-pick-bot[bot])

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v3.2.7...v3.2.8

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>



<!-- risk-assessed -->
