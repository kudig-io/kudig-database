---
title: argo-cd v2.9 Release Notes
description: argo-cd v2.9 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v2.9 Release Notes 是什么
- 如何 argo-cd v2.9 Release Notes
trigger_keywords:
- argo-cd
- v2.9
- Release
- Notes
- release
- notes
---

# argo-cd v2.9 Release Notes

Source: [v2.9.22](https://github.com/argoproj/argo-cd/releases/tag/v2.9.22)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.9.22/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.9.22/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All Argo CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 90c83dc0c32238a332dab0bc1a8bfb66faddc25d: fix: ArgoCD 2.11 - Loop of PATCH calls to Application objects (#19340) (#19569) (@alexmt)
* dd3545b79b0f2832024ce11d7d42edcec1ab2315: fix: docs version regex changed (#18756) (#19356) (@ft-jasong)
### Other work
* 73f9171107350df95d85e5fffbf9cf8def7287a7: upgrade github.com/hashicorp/go-retryablehttp to v0.7.7 (#19239) (@Mangaal)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.9.21...v2.9.22

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

