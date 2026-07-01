---
title: argo-cd v2.5 Release Notes
description: argo-cd v2.5 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v2.5 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v2.5 Release Notes 是什么
- 如何 argo-cd v2.5 Release Notes
trigger_keywords:
- argo-cd
- v2.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---



# argo-cd v2.5 Release Notes

Source: [v2.5.22](https://github.com/argoproj/argo-cd/releases/tag/v2.5.22)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.5.22/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.5.22/manifests/ha/install.yaml
```

## Release signatures

All [[Argo|Argo]] CD container images and CLI binaries are signed by cosign. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets/) on how to verify the signatures.
```shell
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEesHEB7vX5Y2RxXypjMy1nI1z7iRG
JI9/gt/sYqzpsa65aaNP4npM43DDxoIy/MQBo9s/mxGxmA+8UXeDpVC9vw==
-----END PUBLIC KEY-----
```

## 2.5.x has reached EOL

This is the **last patch release in the 2.5 series**. Please upgrade to >=2.6 to continue to receive security updates. Read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation before upgrading.

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changes

This release includes 10 contributions from 4 contributors with 0 features and 4 bug fixes.

### Bug fixes (4)

- fix(controller): cache deadlock on delete and re-add cluster (cherry-pick #14780) (#14815)
- fix(sso): Set redirectURI for gitea, google, oauth [[Dex|Dex]] connectors (#11237) (#14739)
- fix(server): handle PATCH in http/s server (#2677) (#14530) (#14734)
- fix: ApplicationSet Controller crashes when tag is not closed; panic: Cannot find end tag="}}"(#14227) ( #14227) (#14689) (#14691)

### Documentation (2)

- docs: add ignoreDifferences name and namespace fields (#14741) (#14808)
- docs: Skip `export` keyword in notification docs (#14633) (#14645)

### Other (4)

- chore: free up less disk space
- chore(ci): free up disk space
-  docs: Change Generator docs for List Generator to note any key/value pairs can be used (#14825) (#14835)
- chore: Print in-cluster svr addr disabled warning when server starts (#14683)

