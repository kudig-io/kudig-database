---
title: cni-plugins v1.3 Release Notes
description: cni-plugins v1.3 Release Notes — Kubernetes 生产运维知识库
summary: cni-plugins v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cni-plugins v1.3 Release Notes 是什么
- 如何 cni-plugins v1.3 Release Notes
trigger_keywords:
- cni-plugins
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cni-plugins v1.3 Release Notes

Source: [v1.3.0](https://github.com/containernetworking/plugins/releases/tag/v1.3.0)

This release introduces a **new plugin**: `tap`. Thanks to @mmirecki for contributing this

New features:
- ([#784](https://github.com/containernetworking/plugins/pull/784)). tap: This PR adds a plugin to create tap devices.
- ([#829](https://github.com/containernetworking/plugins/pull/829)). bridge: add vlan trunk support
- ([#875](https://github.com/containernetworking/plugins/pull/875)). bridge: Add parameter to disable default vlan
- ([#814](https://github.com/containernetworking/plugins/pull/814)). macvlan: Add support for in-container master
- ([#813](https://github.com/containernetworking/plugins/pull/813)). ipvlan: Add support for in-container master
- ([#781](https://github.com/containernetworking/plugins/pull/781)). vlan: Add support for in-container master

Improvements:
- ([#880](https://github.com/containernetworking/plugins/pull/880)). bridge: read only required chain on cni del instead of the entire ruleset
- ([#873](https://github.com/containernetworking/plugins/pull/873)). bridge, spoof check: remove drop rule index

Bug fixes:
- ([#892](https://github.com/containernetworking/plugins/pull/892)). sbr: Ignore LinkNotFoundError during cmdDel   null
- ([#887](https://github.com/containernetworking/plugins/pull/887)). ptp: Fix ValidateExpectedRoute with non default routes and nil GW
- ([#885](https://github.com/containernetworking/plugins/pull/885)). tuning: fix cmdCheck when using IFNAME
- ([#831](https://github.com/containernetworking/plugins/pull/831)). Fix overwritten error var in getMTUByName
- ([#821](https://github.com/containernetworking/plugins/pull/821)). Only check or del ipv6 when an IPv6 is configured





<!-- risk-assessed -->
