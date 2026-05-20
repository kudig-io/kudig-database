---
title: cri-o v1.24 Release Notes
description: cri-o v1.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v1.24 Release Notes 是什么
- 如何 cri-o v1.24 Release Notes
trigger_keywords:
- cri-o
- v1.24
- Release
- Notes
- release
- notes
---

# cri-o v1.24 Release Notes

Source: [v1.24.6](https://github.com/cri-o/cri-o/releases/tag/v1.24.6)

- [CRI-O v1.24.6](#cri-o-v1246)
  - [Downloads](#downloads)
  - [Changelog since v1.24.5](#changelog-since-v1245)
    - [Changes by Kind](#changes-by-kind)
      - [Bug or Regression](#bug-or-regression)
      - [Uncategorized](#uncategorized)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.24.6

The release notes have been generated for the commit range
[v1.24.5...v1.24.6](https://github.com/cri-o/cri-o/compare/v1.24.5...v1.24.6) on Wed, 14 Jun 2023 14:07:20 UTC.

**Please note: this is likely the last CRI-O 1.24.z release. Please upgrade to a supported version.**

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.v1.24.6.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.24.6.tar.gz)
- [cri-o.amd64.v1.24.6.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.24.6.tar.gz.sha256sum)

## Changelog since v1.24.5

### Changes by Kind

#### Bug or Regression
 - Fix a bug where network metrics collection is broken with systemd cgroup driver and dropped infra containers. ([#6949](https://github.com/cri-o/cri-o/pull/6949), [@haircommander](https://github.com/haircommander))
 - Fixed bug where CRI-O runs with umask of `0`. ([#6901](https://github.com/cri-o/cri-o/pull/6901), [@saschagrunert](https://github.com/saschagrunert))

#### Uncategorized
 - Adds debug log to identify when a relabel was not requested ([#6965](https://github.com/cri-o/cri-o/pull/6965), [@openshift-cherrypick-robot](https://github.com/openshift-cherrypick-robot))

## Dependencies

### Added
_Nothing has changed._

### Changed
- github.com/containers/common: [161e078 → 1fce505](https://github.com/containers/common/compare/161e078...1fce505)

### Removed
_Nothing has changed._
