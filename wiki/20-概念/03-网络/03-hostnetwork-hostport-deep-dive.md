---
sources:
- "网络/K8s网络核心/54-hostnetwork-hostport-deep-dive.md"
title: hostNetwork 与 hostPort 深度解析
summary: 解析 Kubernetes hostNetwork/hostPort 的网络栈绕过机制、端口冲突调度、DNS 策略与安全权衡。
category: concepts
tags:
- hostnetwork
- hostport
- networking
- dns-policy
- daemonset
- security-context
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 网络架构师
- SRE
estimated_read_time: 18min
intent_queries:
- hostNetwork 是什么
- hostPort 与 hostNetwork 区别
- hostNetwork Pod DNS 不工作怎么办
- hostNetwork 端口冲突如何排查
trigger_keywords:
- hostNetwork
- hostPort
- 主机网络
- dnsPolicy
- ClusterFirstWithHostNet
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/05-网络/01-K8s网络核心/56-hostnetwork-hostport-deep-dive.md|hostNetwork 与 hostPort 深度解析]]（本页为重排去重桩，正文见主场）
