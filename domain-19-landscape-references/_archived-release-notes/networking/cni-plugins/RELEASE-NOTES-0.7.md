---
title: cni-plugins v0.7 Release Notes
description: cni-plugins v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cni-plugins v0.7 Release Notes 是什么
- 如何 cni-plugins v0.7 Release Notes
trigger_keywords:
- cni-plugins
- v0.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cni-plugins v0.7 Release Notes

Source: [v0.7.6](https://github.com/containernetworking/plugins/releases/tag/v0.7.6)

This is a bugfix release of the v0.7 train for CNI. It includes one change:

#369 Don't fail when two plugins try to configure the same address at the same time