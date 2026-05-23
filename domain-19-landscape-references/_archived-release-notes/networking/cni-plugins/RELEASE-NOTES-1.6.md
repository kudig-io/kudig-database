---
title: cni-plugins v1.6 Release Notes
description: cni-plugins v1.6 Release Notes — Kubernetes 生产运维知识库
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
- cni-plugins v1.6 Release Notes 是什么
- 如何 cni-plugins v1.6 Release Notes
trigger_keywords:
- cni-plugins
- v1.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cni-plugins v1.6 Release Notes

Source: [v1.6.2](https://github.com/containernetworking/plugins/releases/tag/v1.6.2)

(administrative note: release v1.6.1 was somehow double-created in GitHub; v1.6.2 is identical but fixes the duplication.)

## What's Changed
* portmap: fix nftables backend by @champtar in https://github.com/containernetworking/plugins/pull/1116
* ipmasq: fix nftables backend by @champtar in https://github.com/containernetworking/plugins/pull/1120
* portmap: fix iptables conditions detection by @champtar in https://github.com/containernetworking/plugins/pull/1117


**Full Changelog**: https://github.com/containernetworking/plugins/compare/v1.6.0...v1.6.2