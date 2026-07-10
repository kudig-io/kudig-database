---
title: thanos v0.1 Release Notes
description: thanos v0.1 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.1 Release Notes 是什么
- 如何 thanos v0.1 Release Notes
trigger_keywords:
- thanos
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Thanos|thanos]] v0.1 Release Notes

Source: [v0.1.0](https://github.com/thanos-io/thanos/releases/tag/v0.1.0)

First Thanos *minor* release. 
This is still not major version, so backward compatibility is *NOT* guarnteed.  See: https://semver.org/#spec-item-4.

See changelog](./CHANGELOG.md) for changes.

The major changes in comparison to v0.1.0-rc.2: 
* Updated deps
* Added customizable bucket retention per resolution
* Added optional flag to disable downsampling
* Docs updates
* Improved err handling
* [breaking flag compatibility] Changed store gateway `tsdb.path` flag to `data-dir`
* Proposed gossip removal
* Added IPv6 support
* Extended bucket ls command
* Pinned prometheud dependency (including e2e tests)
* Added support for multiple rule dirs 
* Added support for getting AWS credentials for on-node IAM
* Improved logging
* [breaking flag compatibility] Changed time duration parsing to be same as [[Prometheus|Prometheus]] one (`model.Duration`). Example change: `1m0s` won't work, while `1m` will work.
* Fixed edge case downsampling bug
* Added thanosbench
* Added Thanos Rule UI

<!-- risk-assessed -->
