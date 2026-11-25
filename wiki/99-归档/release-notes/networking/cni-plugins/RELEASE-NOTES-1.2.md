---
title: cni-plugins v1.2 Release Notes
description: cni-plugins v1.2 Release Notes — Kubernetes 生产运维知识库
summary: cni-plugins v1.2 Release Notes — Kubernetes 生产运维知识库
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
- cni-plugins v1.2 Release Notes 是什么
- 如何 cni-plugins v1.2 Release Notes
trigger_keywords:
- cni-plugins
- v1.2
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




# cni-plugins v1.2 Release Notes

Source: [v1.2.0](https://github.com/containernetworking/plugins/releases/tag/v1.2.0)

# Changelog:

## New plugins & features
- ([#743](https://github.com/containernetworking/plugins/pull/743)). dummy: Create a Dummy CNI plugin that creates a virtual interface 
- ([#725](https://github.com/containernetworking/plugins/pull/725)). V2 API support for win-overlay CNI
- ([#693](https://github.com/containernetworking/plugins/pull/693)). tuning Add sysctl allowList

## Bug fixes
- ([#809](https://github.com/containernetworking/plugins/pull/809)). bridge: refresh host-veth mac after port add
- ([#802](https://github.com/containernetworking/plugins/pull/802)). Add IPv6 support for AddDefaultRoute
- ([#779](https://github.com/containernetworking/plugins/pull/779)). Fix path substitution to enable setting sysctls on vlan interfaces
- ([#782](https://github.com/containernetworking/plugins/pull/782)). host-local: fix bug on getting NextIP of addresses with first byte
- ([#709](https://github.com/containernetworking/plugins/pull/709)). dhcp: Fix client id in renew/release

## Improvements & Cleanups:
- ([#772](https://github.com/containernetworking/plugins/pull/772)). portmap support masquerade all
- ([#733](https://github.com/containernetworking/plugins/pull/733)). bridge: support IPAM DNS settings
- ([#702](https://github.com/containernetworking/plugins/pull/702)). bridge:  call ipam.ExecDel after clean up device in netns #702 
- ([#768](https://github.com/containernetworking/plugins/pull/768)). dhcp: Cleanup Socket and Pidfile on exit
- ([#792](https://github.com/containernetworking/plugins/pull/792)). dhcp: Update Allocate method to reuse lease if present
- ([#755](https://github.com/containernetworking/plugins/pull/755)). dhcp: Use the same options for acquiring, renewing lease
- ([#730](https://github.com/containernetworking/plugins/pull/730)). tuning Check for duplicated sysctl keys
- ([#739](https://github.com/containernetworking/plugins/pull/739)). build: support riscv64
- ([#712](https://github.com/containernetworking/plugins/pull/712)). bug: return errors when iptables and ip6tables are unusable
- ([#719](https://github.com/containernetworking/plugins/pull/719)). Make description for `static` plugin more exact


As always, many thanks to our contributors.

<!-- risk-assessed -->
