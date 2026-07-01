---
title: coredns v1.14 Release Notes
description: coredns v1.14 Release Notes — Kubernetes 生产运维知识库
summary: coredns v1.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- coredns
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- coredns v1.14 Release Notes 是什么
- 如何 coredns v1.14 Release Notes
trigger_keywords:
- coredns
- v1.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[CoreDNS|coredns]] v1.14 Release Notes

Source: [v1.14.2](https://github.com/coredns/coredns/releases/tag/v1.14.2)

This release adds the new proxyproto plugin to support Proxy Protocol and preserve
client IPs behind load balancers. It also includes enhancements such as improved DNS
logging metadata and stronger randomness for loop detection (CVE-2026-26018), along
with several bug fixes including TLS+IPv6 forwarding, improved CNAME handling and
rewriting, allowing jitter disabling, prevention of an ACL bypass (CVE-2026-26017),
and a [[Kubernetes|Kubernetes]] plugin crash fix. In addition, the release updates the build to
Go 1.26.1, which include security fixes addressing CVE-2026-27137, CVE-2026-27138, CVE-2026-27139,
CVE-2026-25679, and CVE-2026-27142.

## Brought to You By

Adphi
Henrik Gerdes
hide
Kelly Kane
Shiv Tyagi
vflaux
Ville Vesilehto
yangsenzk
Yong Tang
YOUNEVSKY

## Noteworthy Changes

* core: Reorder rewrite before acl to prevent bypass (https://github.com/coredns/coredns/pull/7882)
* plugin/file: Return SOA and NS records when queried for a record CNAMEd to origin (https://github.com/coredns/coredns/pull/7808)
* plugin/forward: Fix parsing error when handling TLS+IPv6 address (https://github.com/coredns/coredns/pull/7848)
* plugin/log: Add metadata for response Type and Class to Log (https://github.com/coredns/coredns/pull/7806)
* plugin/loop: Use crypto/rand for query name generation (https://github.com/coredns/coredns/pull/7881)
* plugin/kubernetes: Fix panic on empty ListenHosts (https://github.com/coredns/coredns/pull/7857)
* plugin/proxyproto: Add proxy protocol support (https://github.com/coredns/coredns/pull/7738)
* plugin/reload: Allow disabling jitter with 0s (https://github.com/coredns/coredns/pull/7896)
* plugin/rewrite: Fix cname target rewrite for CNAME chains (https://github.com/coredns/coredns/pull/7853)
