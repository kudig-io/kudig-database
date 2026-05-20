---
title: argo-cd v2.14 Release Notes
description: argo-cd v2.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
- redis
- operator
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v2.14 Release Notes 是什么
- 如何 argo-cd v2.14 Release Notes
trigger_keywords:
- argo-cd
- v2.14
- Release
- Notes
- release
- notes
---

# argo-cd v2.14 Release Notes

Source: [v2.14.21](https://github.com/argoproj/argo-cd/releases/tag/v2.14.21)

> [!IMPORTANT]
> **END OF LIFE NOTICE**
> 
> This is the final release of the 2.14.x release series. As of November 4, 2025, this version has reached end of life and will no longer receive bug fixes or security updates.
> 
> **Action Required**: Please upgrade to a [supported version](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) (v3.0 or higher).

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.14.21/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.14.21/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All Argo CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 8b315440697956e1e128b6a61c5653c08c299e11: fix: make webhook payload handlers recover from panics (cherry-pick #24862 for 2.14) (#24926) (@jake-ciolek)
### Dependency updates
* 9b7bf3e16d17f0cb14034783c444b9a6e35bca4f: chore(deps): bump redis from 7.0.14 to 7.2.11 to address vuln (release-2.14) (#24892) (@carlosrodfern)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.14.20...v2.14.21

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

