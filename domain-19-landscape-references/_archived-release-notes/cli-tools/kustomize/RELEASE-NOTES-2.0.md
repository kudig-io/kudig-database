---
title: kustomize v2.0 Release Notes
description: kustomize v2.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- crd
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kustomize v2.0 Release Notes 是什么
- 如何 kustomize v2.0 Release Notes
trigger_keywords:
- kustomize
- v2.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kustomize v2.0 Release Notes

Source: [v2.0.3](https://github.com/kubernetes-sigs/kustomize/releases/tag/v2.0.3)

## Changelog

45ba7856 Add [[ConfigMaps|configmaps]] test for json string
8bbe147c Add webhooks to order list of gvk
ea3d5e68 Fix for #818 - Added support for quoted values
eb752039 Fix for #831 - Ignore domain when finding the image tag
6bfd7cff Improve error handling during var resolution.
ed2ad860 Move trim quotes logic to separate function
ff6cd3ca Report unused variables.
1303ea39 Run kustomize tests on OSX
e666630d Simplify map conversion logic
9d77cbea Update golang/x/net dependency to release-branch.go1.11
28cefb3b improve error message for loading files listed under crds
78cbff16 improve error message in json patch transformer
b0c3cd75 update the doc for crds: the files in this list should be openAPI definition
f4eef1dc update transformerconfigs/crd example
