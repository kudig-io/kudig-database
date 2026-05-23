---
title: thanos v0.1 Release Notes
description: thanos v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- gateway
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
created: "2026-05-23"
---

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