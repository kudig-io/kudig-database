---
title: helm v1.2 Release Notes
description: helm v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v1.2 Release Notes 是什么
- 如何 helm v1.2 Release Notes
trigger_keywords:
- helm
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

# helm v1.2 Release Notes

Source: [v1.2.1](https://github.com/helm/helm/releases/tag/v1.2.1)

This revision of the 1.2 release fixes a bug that causes make to fail when building and pushing the docker images for the dm server side components. The binaries for this release are pushed to gcr.io/dm-k8s-prod with tag v1.2.1 and to gcr.io/get-dm.
