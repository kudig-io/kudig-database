---
title: containerd v0.0 Release Notes
description: containerd v0.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd v0.0 Release Notes 是什么
- 如何 containerd v0.0 Release Notes
trigger_keywords:
- containerd
- v0.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# containerd v0.0 Release Notes

Source: [0.0.5](https://github.com/containerd/containerd/releases/tag/0.0.5)

This release has support for runc by default and adds features such as detach where containerd can die and reattach to all containers including exit events and stdio.

You will need runc installed on your system to use conatinerd.

You can find all the downloads you need to runc containerd on a linux amd64 system included in this release.   Just copy all binaries to your PATH and runc in your path under the name `runc`
