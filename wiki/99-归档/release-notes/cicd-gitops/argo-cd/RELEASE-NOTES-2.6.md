---
title: argo-cd v2.6 Release Notes
description: argo-cd v2.6 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v2.6 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v2.6 Release Notes 是什么
- 如何 argo-cd v2.6 Release Notes
trigger_keywords:
- argo-cd
- v2.6
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




# argo-cd v2.6 Release Notes

Source: [v2.6.15](https://github.com/argoproj/argo-cd/releases/tag/v2.6.15)

## Quick Start

### Non-HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.6.15/manifests/install.yaml
```
### HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.6.15/manifests/ha/install.yaml
```
## Release signatures

All [[Argo|Argo]] CD container images and CLI binaries are signed by cosign. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets/) on how to verify the signatures.
```shell
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEesHEB7vX5Y2RxXypjMy1nI1z7iRG
JI9/gt/sYqzpsa65aaNP4npM43DDxoIy/MQBo9s/mxGxmA+8UXeDpVC9vw==
-----END PUBLIC KEY-----
```

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changes

This release fixes two security issues:

* [CVE-2023-40029](https://github.com/argoproj/argo-cd/security/advisories/GHSA-fwr2-64vr-xv9m)
* [CVE-2023-40584](https://github.com/argoproj/argo-cd/security/advisories/GHSA-g687-f2gx-6wm8)

## Security (2)

- Merge pull request from GHSA-fwr2-64vr-xv9m
- Merge pull request from GHSA-g687-f2gx-6wm8

### Bug fixes (0)

### Documentation (2)

- docs: document sourceNamespaces field (#15195) (#15214)
- chore: add example jq path expression (#15130) (#15211)

### Other (0)



<!-- risk-assessed -->
