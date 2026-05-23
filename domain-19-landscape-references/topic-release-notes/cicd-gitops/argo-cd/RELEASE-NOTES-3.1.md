---
title: argo-cd v3.1 Release Notes
description: argo-cd v3.1 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v3.1 Release Notes 是什么
- 如何 argo-cd v3.1 Release Notes
trigger_keywords:
- argo-cd
- v3.1
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

# argo-cd v3.1 Release Notes

Source: [v3.1.13](https://github.com/argoproj/argo-cd/releases/tag/v3.1.13)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.1.13/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.1.13/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All [[Argo|Argo]] CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.

## Release Notes Blog Post
For a detailed breakdown of the key changes and improvements in this release, check out the [official blog post](https://blog.argoproj.io/announcing-argo-cd-v3-1-f4389bc783c8)

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 7a7dd4e5a56cd33c51bf2b4b84f5e7ee205d2eeb: fix(UI): show RollingSync step clearly when labels match no step (cherry-pick #26877 for 3.1) (#26885) (@argo-cd-cherry-pick-bot[bot])
* beb9860db59bf56409f057c206786352435d3d81: fix: mitigation of grpc-go CVE-2026-33186 for release-3.1 (#26982) (@dudinea)
### Dependency updates
* c19d63446deb1aca568687acad5e7cfff8711809: chore(deps): bump lodash from 4.17.21 to 4.17.23 (Cherry-pick 3.1)  (#26210) (@nmirasch)
### Other work
* 12c8d42f4f4ede97a3da1dabacbf312ad0b0efd3: chore: use base ref for cherry-pick prs (cherry-pick #26551 for 3.1) (#26555) (@argo-cd-cherry-pick-bot[bot])

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v3.1.12...v3.1.13

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

