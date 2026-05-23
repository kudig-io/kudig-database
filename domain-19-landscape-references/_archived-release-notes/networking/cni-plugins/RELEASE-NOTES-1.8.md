---
title: cni-plugins v1.8 Release Notes
description: cni-plugins v1.8 Release Notes — Kubernetes 生产运维知识库
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
- cni-plugins v1.8 Release Notes 是什么
- 如何 cni-plugins v1.8 Release Notes
trigger_keywords:
- cni-plugins
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cni-plugins v1.8 Release Notes

Source: [v1.8.0](https://github.com/containernetworking/plugins/releases/tag/v1.8.0)

The Bridge CNI plugin has removed limitations on VLAN trunk implementation. This aligns with recommended access and trunk port configurations, ensuring proper VLAN isolation and enhanced usability.

## What's Changed
* Allow vlan parameter to set native vlan on trunk ports by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1180
* Set default value of PreserveDefaultVlan to False by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1181
* remove duplicate route.Table and route.Scope assignments by @runsisi in https://github.com/containernetworking/plugins/pull/1192
* Set value of gw to nil for opt121 routes in DHCP by @omartin2010 in https://github.com/containernetworking/plugins/pull/1187

## New Contributors
* @runsisi made their first contribution in https://github.com/containernetworking/plugins/pull/1192
* @omartin2010 made their first contribution in https://github.com/containernetworking/plugins/pull/1187

**Full Changelog**: https://github.com/containernetworking/plugins/compare/v1.7.0...v1.8.0