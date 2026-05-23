---
title: argo-cd v2.7 Release Notes
description: argo-cd v2.7 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v2.7 Release Notes 是什么
- 如何 argo-cd v2.7 Release Notes
trigger_keywords:
- argo-cd
- v2.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
created: "2026-05-23"
---

# argo-cd v2.7 Release Notes

Source: [v2.7.18](https://github.com/argoproj/argo-cd/releases/tag/v2.7.18)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.7.18/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.7.18/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All [[Argo|Argo]] CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* ff439f6c60579fba2acbe39cf1cb434babc6a710: fix: fix calculating patch for respect ignore diff feature (#17693) (#17733) (@alexmt)
### Other work
* f45ef020d6a8e50c225a99573a662e034036b833: fix cosign (#17656) (#17740) (@alexmt)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.7.17...v2.7.18

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

