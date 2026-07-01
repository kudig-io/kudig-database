---
title: coredns v1.10 Release Notes
description: coredns v1.10 Release Notes — Kubernetes 生产运维知识库
summary: coredns v1.10 Release Notes — Kubernetes 生产运维知识库
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
- coredns v1.10 Release Notes 是什么
- 如何 coredns v1.10 Release Notes
trigger_keywords:
- coredns
- v1.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[CoreDNS|coredns]] v1.10 Release Notes

Source: [v1.10.1](https://github.com/coredns/coredns/releases/tag/v1.10.1)

This release fixes some bugs, and adds some new features including:
* Corrected architecture labels in multi-arch image manifest
* A new plugin *timeouts* that allows configuration of server listener timeout durations
* *acl* can drop queries as an action
* *template* supports creating responses with extended DNS errors
* New weighted policy in *loadbalance*
* Option to serve original record TTLs from *cache*

## Brought to You By

Arthur Outhenin-Chalandre,
Ben Kaplan,
Chris O'Haver,
Gabor Dozsa,
Grant Spence,
Kumiko as a [[Service|Service]],
LAMRobinson,
Miciah Dashiel Butler Masters,
Ondřej Benkovský,
Rich,
Stephen Kitt,
Yash Singh,
Yong Tang,
rsclarke,
sanyo0714

## Noteworthy Changes

* plugin/timeouts - Allow ability to configure listening server timeouts (https://github.com/coredns/coredns/pull/5784)
* plugin/acl: adding ability to drop queries (https://github.com/coredns/coredns/pull/5722)
* plugin/template : add support for extended DNS errors (https://github.com/coredns/coredns/pull/5659)
* plugin/kubernetes: error NXDOMAIN for TXT lookups (https://github.com/coredns/coredns/pull/5737)
* plugin/kubernetes: dont match external services when endpoint is specified (https://github.com/coredns/coredns/pull/5734)
* plugin/k8s_external: Fix rcode for headless services (https://github.com/coredns/coredns/pull/5657)
* plugin/edns: remove truncating of question section on bad EDNS version (https://github.com/coredns/coredns/pull/5787)
* plugin/dnstap: Fix behavior when multiple dnstap plugins specified (https://github.com/coredns/coredns/pull/5773)
* plugin/cache: cache now uses source query DNSSEC option for upstream refresh (https://github.com/coredns/coredns/pull/5671)
* Workaround for incorrect architecture (https://github.com/coredns/coredns/pull/5691)
* plugin/loadbalance: Add weighted policy (https://github.com/coredns/coredns/pull/5662)
* plugin/cache: Add keepttl option (https://github.com/coredns/coredns/pull/5879)
* plugin/forward: Fix dnstap for forwarded request/response (https://github.com/coredns/coredns/pull/5890)

**Full Changelog**: https://github.com/coredns/coredns/compare/v1.10.0...v1.10.1