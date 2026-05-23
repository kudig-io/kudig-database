---
title: kustomize v3.0 Release Notes
description: kustomize v3.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kustomize v3.0 Release Notes 是什么
- 如何 kustomize v3.0 Release Notes
trigger_keywords:
- kustomize
- v3.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kustomize v3.0 Release Notes

Source: [v3.0.3](https://github.com/kubernetes-sigs/kustomize/releases/tag/v3.0.3)

## Changelog

bfafbbf4 Add FAQ about how to customize configuration
fb44880b Add back GCP KMS example
08d7c35d Add storage class name ref
580963ea Address replacement of digest by ImageTransformer
579995dc Address simultaneous transformation of name and namespace
7998ee70 Addresses slice case with notNamespaceable objects
f1dbab9d Convert go plugin example to GPG based
0edab60b Fix typo: kubectl v1.15 -> kubectl v1.14 (#1333)
9b40f8ab Implement code review comments to NameReferenceTransformer changes.
c4d899f7 Improve NameReference Test cases
0d8d9e2f Move plugin EnvForTest manager into new package
e5ebca66 Test tracking issue "patchesStrategicMerge elements can be dropped"
b43bd544 Update Issue 1264 Reproduction Test
c3ea109b Update goPluginGuidedExample.md
095333ff Update references to NewEnvForTest
3c05e2d6 add extended patch transformer
ed0cfc68 add test for extended patch with overlapping patches
120ba6b8 docs/versioningPolicy.md: fix expired urls
a85f297f enable extended patch transformer and add tests
6f744196 fix local test failures
f5fc9acb fix local test failures
8121467c fix the ci failure
28d1bad3 fix the ci failure
dc6dcd81 update the latest version in readme
