---
title: argo-cd v3.2 Release Notes
description: argo-cd v3.2 Release Notes — Kubernetes 生产运维知识库
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

# argo-cd v3.2 Release Notes

Source: [v3.2.8](https://github.com/argoproj/argo-cd/releases/tag/v3.2.8)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.2.8/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.2.8/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All Argo CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.

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

