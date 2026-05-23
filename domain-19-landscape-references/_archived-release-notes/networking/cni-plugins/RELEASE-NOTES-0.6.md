---
title: cni-plugins v0.6 Release Notes
description: cni-plugins v0.6 Release Notes — Kubernetes 生产运维知识库
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
- cni-plugins v0.6 Release Notes 是什么
- 如何 cni-plugins v0.6 Release Notes
trigger_keywords:
- cni-plugins
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cni-plugins v0.6 Release Notes

Source: [v0.6.0](https://github.com/containernetworking/plugins/releases/tag/v0.6.0)

This is the first release of the CNINI Plugins|CNI plugins]] project. It includes all of the plugins formerly part of the [cni](https://github.com/containernetworking/cni) repository, along with a new portmapping plugin.

🎉 It also includes IPv6 support for all interface plugins 🎉.

All of the plugins support version 0.3.1 of the CNI spec.

Notable changes since the previous CNI release:
* #12, #50 ipam/host-local: support multiple IP ranges
* #10: Bridge: add IPv6 support
* #25 ptp: add IPv6 support
* #35 bridge: add support for promiscuous mode
* #47 tuning: support plugin chaining