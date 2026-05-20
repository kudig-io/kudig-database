---
title: argo-cd v2.11 Release Notes
description: argo-cd v2.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v2.11 Release Notes 是什么
- 如何 argo-cd v2.11 Release Notes
trigger_keywords:
- argo-cd
- v2.11
- Release
- Notes
- release
- notes
---

# argo-cd v2.11 Release Notes

Source: [v2.11.14](https://github.com/argoproj/argo-cd/releases/tag/v2.11.14)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.11.14/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.11.14/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All Argo CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 968b05f5bbff9a02d2b4d3f9beaeeec685e28d97: fix(security): repository.GetDetailedProject exposes repo secrets (#24387) (#24463) (@alexmt)
### Dependency updates
* b2a4aee86486298a4184dbe3b25f5774391fb050: chore(deps): bump slsa-framework/slsa-github-generator from 2.0.0 to 2.1.0 (#23166) (#24470) (@alexmt)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.11.13...v2.11.14

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

