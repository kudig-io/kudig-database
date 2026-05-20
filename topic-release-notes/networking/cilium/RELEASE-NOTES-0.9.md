---
title: cilium v0.9 Release Notes
description: cilium v0.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- cilium
- docker
- daemonset
- ingress
- rbac
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v0.9 Release Notes 是什么
- 如何 cilium v0.9 Release Notes
trigger_keywords:
- cilium
- v0.9
- Release
- Notes
- release
- notes
---

# cilium v0.9 Release Notes

Source: [v0.9.0](https://github.com/cilium/cilium/releases/tag/v0.9.0)

Features
--------

- Core

  - New simplified policy language (#670)
  - Option to choose between a global (#default) and per endpoint connection tracking table (#659)
  - Parallel endpoint BPF program & policy builds (#424, #587)
  - Fluentd logging integration (#758)
  - IPv6 proxy redirection support (#818)
  - Transparent ingress proxy redirection (#773)
  - Consider all labels for identity except dynamic k8s state labels (#849)
  - Reduced size of cilium binary from 27M to 17M (#554)
  - Add filtering support to ``cilium monitor`` (#673)
  - Allow rule now supports matching multiple labels (#638)
  - Separate runtime state and template directory for security reasons (#537)
  - Ability to specify L4 destination port in policy trace (#650)
  - Improved log readability (#499)
  - Optimized connection tracking map updates per packet (#829)
  - New ``--kvstore`` and ``--kvstore-opt`` flag (Replaces ``--consul, --etcd, --local`` flags)  (#767)
  - Configurable clang path (#620)
  - Updated CNI to 5.2.0 (#529)
  - Updated Golang to 1.8.3 (#853)
  - Bump k8s client to v3.0.0-beta.0 (#646)

- Kubernetes

  - Support L4 filtering with v1beta1.NetworkPolicyPort (#638)
  - ThirdPartyResources support for L3-L7 policies (#795, #814)
  - Per pod policy enablement based on policy selection (#815)
  - Support for full LabelSelector (#753)
  - Option to always allow localhost to reach endpoints (#auto on with k8s) (#754)
  - RBAC ClusterRole, ServiceAccount and bindings (#850)
  - Scripts to install and uninstall CNI configuration (#745)

- Documentation

  - Getting started guide for minikube (#734)
  - Kubernetes installation guide using DaemonSet (#800)
  - Rework of the administrator guide (#850)
  - New simplified vagrant box to get started (#549)
  - API reference documentation (#512)
  - BPF & XDP documentation (#546)

Fixes
-----

- Core

  - Endpoints are displayed in ascending order (#474)
  - Warn about insufficient kernel version when starting up (#505)
  - Work around Docker <17.05 disabling IPv6 in init namespace (#544)
  - Fixed a connection tracking expiry a bug (#828)
  - Only generate human readable ASM output if DEBUG is enabled (#599)
  - Switch from package syscall to x/sys/unix (#588)
  - Remove tail call map on endpoint leave (#736)
  - Fixed ICMPv6 to service IP with LB back to own IP (#764)
  - Respond to ARP also when temporary drop all policy is applied. (#724)
  - Fixed several BPF resource leakages (#634, #684, #732)
  - Fixed several L7 parser policy bugs (#512)
  - Fixed tc call to specify prio and handle for replace (#611)
  - Fixed off by one in consul connection retries (#610)
  - Fixed lots of documentation typos
  - Fix addition/deletion order when updating endpoint labels (#647)
  - Graceful exit if lack of privileges (#694)
  - use same tuple struct for both global and local CT (#822)
  - bpf/init.sh: More robust deletion of routes. (#719)
  - lxc endianess & src validation fixes (#747)

- Kubernetes

  - Correctly handle k8s NetworkPolicy matchLabels (#638)
  - Allow all sources if []NetworkPolicyPeer is empty or missing (#638)
  - Fix if k8s API server returns nil label (#567)
  - Do not error out if k8s node does not have a CIDR assigned (#628)
  - Only attempt to resolve CIDR from k8s API if client is available (#608)
  - Log error if invalid k8s NetworkPolicy objects are received (#617)