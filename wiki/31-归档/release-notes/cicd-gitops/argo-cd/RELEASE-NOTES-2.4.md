---
title: argo-cd v2.4 Release Notes
description: argo-cd v2.4 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v2.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v2.4 Release Notes 是什么
- 如何 argo-cd v2.4 Release Notes
trigger_keywords:
- argo-cd
- v2.4
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




# argo-cd v2.4 Release Notes

Source: [v2.4.28](https://github.com/argoproj/argo-cd/releases/tag/v2.4.28)

## Quick Start

### Non-HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.4.28/manifests/install.yaml
```
### HA:

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.4.28/manifests/ha/install.yaml
```
## 2.4.x has reached EOL

This is the **last patch release in the 2.4 series**. Please upgrade to >=2.5 to continue to receive security updates. Read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation before upgrading.

## Breaking changes

As part of the fix for https://github.com/argoproj/argo-cd/security/advisories/GHSA-2q5c-qw9c-fmvq, the API will now return "Unauthorized" instead of "Not found" if an Application does not exist. This change prevents leaking the existence or non-existence of Applications to unauthorized parties.

This change may break applications which depend on "Not found" responses from the [[Argo|Argo]] CD API's application endpoints.

Workarounds and potential long-term solutions will be discussed on https://github.com/argoproj/argo-cd/issues/13000.

The `argocd app create` CLI command for versions >= 2.5.0-rc1 and before this security patch is one such application which was affected. (See [upgrade notes](https://argo-cd.readthedocs.io/en/latest/operator-manual/upgrading/2.5-2.6/#argocd-app-create-for-old-cli-versions-fails-with-api-version-267) for details on that issue.)


## Release signatures

All Argo CD container images and CLI binaries are signed by cosign. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets/) on how to verify the signatures.
```shell
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEesHEB7vX5Y2RxXypjMy1nI1z7iRG
JI9/gt/sYqzpsa65aaNP4npM43DDxoIy/MQBo9s/mxGxmA+8UXeDpVC9vw==
-----END PUBLIC KEY-----
```

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changes

This release includes 1 contributions from 1 contributors with 0 features and 0 bug fixes.

### Security (1)

- MODERATE: Authenticated but unauthorized users may enumerate Application names via the API (https://github.com/argoproj/argo-cd/security/advisories/GHSA-2q5c-qw9c-fmvq)



<!-- risk-assessed -->
