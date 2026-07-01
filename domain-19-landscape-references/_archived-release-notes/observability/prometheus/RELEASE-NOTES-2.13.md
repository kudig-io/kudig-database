---
title: argo-cd v2.13 Release Notes
description: argo-cd v2.13 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v2.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
- crd
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
- argo-cd v2.13 Release Notes 是什么
- 如何 argo-cd v2.13 Release Notes
trigger_keywords:
- argo-cd
- v2.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---



# argo-cd v2.13 Release Notes

Source: [v2.13.9](https://github.com/argoproj/argo-cd/releases/tag/v2.13.9)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.13.9/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.13.9/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All [[Argo|Argo]] CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* b8ac09d326d728d178a4cc6af6aa4e2a3afb22f3: fix(security): repository.GetDetailedProject exposes repo [[Secrets|secrets]] (#24388) (@crenshaw-dev)
* bfa724719a0861e91151bedfa01fdd99bee57a43: fix(server): infer resource status health for apps-in-any-ns (#22944) (#23708) (@crenshaw-dev)
* df347d0d566c904a282e13add3cda82fed4e37c4: fix: do not normalize resource tracking on live crds (#22722) - cherrypick 2.13 (#22747) (@blakepettersson)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.13.8...v2.13.9

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

