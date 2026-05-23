---
title: cni-plugins v1.0 Release Notes
description: cni-plugins v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flannel
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cni-plugins v1.0 Release Notes 是什么
- 如何 cni-plugins v1.0 Release Notes
trigger_keywords:
- cni-plugins
- v1.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cni-plugins v1.0 Release Notes

Source: [v1.0.1](https://github.com/containernetworking/plugins/releases/tag/v1.0.1)

# CNINI Plugins|CNI Plugins]] v1.0.1 is here

This release adds support for [CNI Spec v1.0](https://github.com/containernetworking/cni/blob/spec-v1.0.0/SPEC.md). Additionally, it officially declares CNI as a stable project.

The Flannel CNI plugin has been moved to a [separate project](https://github.com/flannel-io/cni-plugin), and is no longer included here.

# Changes since v1.0.0 :man_facepalming:  
- plugins: fix bug where support for CNI version 0.4.0 or 1.0.0 was dropped

# Changes since v0.9.1

## :warning:  Breaking Changes
- plugins: remove flannel ([#633](https://github.com/containernetworking/plugins/pull/633)). Flannel's CNI plugin now has [its own repository](https://github.com/flannel-io/cni-plugin)

## :chart_with_upwards_trend: New Features
- bridge: Add mac field to specify container iface mac ([#636](https://github.com/containernetworking/plugins/pull/636)).
- (generic) Allow multiple routes to be added for the same prefix ([#615](https://github.com/containernetworking/plugins/pull/615)). Enables ECMP.
- (sbr): Add multi IP support ([#623](https://github.com/containernetworking/plugins/pull/623)).

## :sparkles: Other improvements
- (generic): place veth peer in host namspace directly ([#645](https://github.com/containernetworking/plugins/pull/645)).
- (windows): refactor win-bridge, support HNSv2 ([#617](https://github.com/containernetworking/plugins/pull/617)).
- (host-local): support ip/prefix in env args and CNI args ([#630](https://github.com/containernetworking/plugins/pull/630)). 
- (host-local): support custom IPs allocation through runtime configuraton ([#599](https://github.com/containernetworking/plugins/pull/599)).
- (tuning): always update MAC in CNI result ([#626](https://github.com/containernetworking/plugins/pull/626)).
- (tuning): Add support of altering the allmulticast flag ([#624](https://github.com/containernetworking/plugins/pull/624)). 

## :bug:   Bug Fixes
- host-local: remove redundant startRange in RangeIterator to avoid mismatching with startIP ([#583](https://github.com/containernetworking/plugins/pull/583)). Fixes possible infinite loop.
- portmap: use slashes in sysctl template to support interface names which separated by dots ([#589](https://github.com/containernetworking/plugins/pull/589)).
- pkg/ipam: convert dots to slashes in interface names for sysctl ([#585](https://github.com/containernetworking/plugins/pull/585)).
- win-bridge: fix panic while calling HNS api ([#590](https://github.com/containernetworking/plugins/pull/590)). fix a nil pointer panic while calling HNS API (V1) on win-bridge.
- [macvlan] Stop setting proxy-arp on macvlan interface ([#586](https://github.com/containernetworking/plugins/pull/586)).


As always, thanks to our dedicated maintainers and contributors!

