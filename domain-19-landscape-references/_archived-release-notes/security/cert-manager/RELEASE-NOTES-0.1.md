---
title: etcd v0.1 Release Notes
description: etcd v0.1 Release Notes — Kubernetes 生产运维知识库
summary: etcd v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd v0.1 Release Notes 是什么
- 如何 etcd v0.1 Release Notes
trigger_keywords:
- etcd
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[etcd|etcd]] v0.1 Release Notes

Source: [v0.1.2](https://github.com/etcd-io/etcd/releases/tag/v0.1.2)

0.1.2 Blog Post: http://coreos.com/blog/etcd-v0.1.2-new-dashboard-and-bugfixes/

Thank you to all of the contributors in this release:

Andrew Hobden, AndyPook, Antonio Terreno, Brandon Philips, David Fisher, Deniz Adrian, Derek Chiang (Enchi Jiang), Diwaker Gupta, Evan, Fabrizio (Misto) Milo, Fatih Arslan, Geoff Hayes, Yifan Gu, Jose Plana, Michael Burns, Michael Marineau, Michael Stillwell, Rob Szumski, Roberto Aguilar, Theo Hultberg, Xiang Li, kelseyhightower


<!-- risk-assessed -->
