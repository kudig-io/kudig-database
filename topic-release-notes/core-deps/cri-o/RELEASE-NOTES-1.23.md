---
title: cri-o v1.23 Release Notes
description: cri-o v1.23 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.23 Release Notes 是什么
- 如何 cri-o v1.23 Release Notes
trigger_keywords:
- cri-o
- v1.23
- Release
- Notes
- release
- notes
---

# cri-o v1.23 Release Notes

Source: [v1.23.5](https://github.com/cri-o/cri-o/releases/tag/v1.23.5)

- [CRI-O v1.23.5](#cri-o-v1235)
  - [Downloads](#downloads)
  - [Changelog since v1.23.4](#changelog-since-v1234)
    - [Changes by Kind](#changes-by-kind)
      - [Bug or Regression](#bug-or-regression)
      - [Uncategorized](#uncategorized)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.23.5

The release notes have been generated for the commit range
[v1.23.4...d9dec98](https://github.com/cri-o/cri-o/compare/v1.23.4...d9dec984d80a4af2edc47822cfd42e8a6a3827ab) on Mon, 23 Jan 2023 08:01:01 UTC.

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz)
- [cri-o.amd64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz.sha256sum)
- [cri-o.arm64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz)
- [cri-o.arm64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.d9dec984d80a4af2edc47822cfd42e8a6a3827ab.tar.gz.sha256sum)

## Changelog since v1.23.4

### Changes by Kind

#### Bug or Regression
 - Fixed bug to restore `/var/lib/containers/storage/overlay/backingFsBlockDev` on XFS file systems. ([#6390](https://github.com/cri-o/cri-o/pull/6390), [@saschagrunert](https://github.com/saschagrunert))

#### Uncategorized
 - Fix a bug about log container ([#6409](https://github.com/cri-o/cri-o/pull/6409), [@laxmanvallandas](https://github.com/laxmanvallandas))

## Dependencies

### Added
_Nothing has changed._

### Changed
- github.com/containers/storage: [v1.37.2 → v1.37.3](https://github.com/containers/storage/compare/v1.37.2...v1.37.3)

### Removed
_Nothing has changed._
