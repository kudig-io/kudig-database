---
title: cert-manager v1.20 Release Notes
description: cert-manager v1.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- gateway
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v1.20 Release Notes 是什么
- 如何 cert-manager v1.20 Release Notes
trigger_keywords:
- cert-manager
- v1.20
- Release
- Notes
- release
- notes
---

# cert-manager v1.20 Release Notes

Source: [v1.20.1](https://github.com/cert-manager/cert-manager/releases/tag/v1.20.1)

v1.20.1 fixes an issue for OpenShift users that has to do with the finalizer RBAC, bumps gRPC to address a reported non-affecting vulnerability, and fixes a duplicate `parentRef` bug when both issuer config and annotations are present (Gateway API).

### Bug or Regression

- Fixed duplicate `parentRef` bug when both issuer config and annotations are present. (#8658, @hjoshi123)
- Add missing issuer finalizer RBAC to the order controller to support owner references. This was preventing OpenShift users from being able to upgrade to v1.20.0. (#8655, @erikgb)
- Bump google.golang.org/grpc to fix vulnerability reported by scanners. This isn't a vulnerability that affects cert-manager, but we are bumping it because it is reported by scanners. (#8657, @erikgb)
