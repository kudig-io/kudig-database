---
title: falco v0.9 Release Notes
description: falco v0.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- falco
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.9 Release Notes 是什么
- 如何 falco v0.9 Release Notes
trigger_keywords:
- falco
- v0.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Falco|falco]] v0.9 Release Notes

Source: [0.9.0](https://github.com/falcosecurity/falco/releases/tag/0.9.0)

Released 2018-01-18

### Bug Fixes

* Fix driver incompatibility problems with some linux kernel versions that can disable pagefault tracepoints [[#sysdig/1034](https://github.com/draios/sysdig/pull/1034)]
* Fix OSX Build incompatibility with latest version of libcurl [[#291](https://github.com/draios/falco/pull/291)]

### Minor Changes

* Updated the [[Kubernetes|Kubernetes]] example to provide an additional example: Daemon Set using RBAC and a ConfigMap for configuration. Also expanded the documentation for both the RBAC and non-RBAC examples. [[#309](https://github.com/draios/falco/pull/309)]

### Rule Changes

* Refactor the shell-related rules to reduce false positives. These changes significantly decrease the scope of the rules so they trigger only for shells spawned below specific processes instead of anywhere. [[#301](https://github.com/draios/falco/pull/301)] [[#304](https://github.com/draios/falco/pull/304)]
* Lots of rule changes based on feedback from Sysdig Secure community [[#293](https://github.com/draios/falco/pull/293)] [[#298](https://github.com/draios/falco/pull/298)] [[#300](https://github.com/draios/falco/pull/300)] [[#307](https://github.com/draios/falco/pull/307)] [[#315](https://github.com/draios/falco/pull/315)]