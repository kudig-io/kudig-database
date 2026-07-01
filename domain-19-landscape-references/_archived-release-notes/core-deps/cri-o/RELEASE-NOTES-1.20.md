---
title: cri-o v1.20 Release Notes
description: cri-o v1.20 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
- docker
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v1.20 Release Notes 是什么
- 如何 cri-o v1.20 Release Notes
trigger_keywords:
- cri-o
- v1.20
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# cri-o v1.20 Release Notes

Source: [v1.20.9](https://github.com/cri-o/cri-o/releases/tag/v1.20.9)

- [CRI-O v1.20.9](#cri-o-v1209)
  - [Downloads](#downloads)
  - Changelog since v1.20.8](#changelog-since-v1208)
    - [Changes by Kind](#changes-by-kind)
      - [Feature](#feature)
      - [Bug or Regression](#bug-or-regression)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.20.9

The release notes have been generated for the commit range [v1.20.7...v1.20.9](https://github.com/cri-o/cri-o/compare/v1.20.7...v1.20.9) on Thu, 21 Jul 2022 08:55:21 CEST.

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.v1.20.9.tar.gz](https://storage.googleapis.com/k8s-conform-cri-o/artifacts/cri-o.amd64.v1.20.9.tar.gz)
- [cri-o.arm64.v1.20.9.tar.gz](https://storage.googleapis.com/k8s-conform-cri-o/artifacts/cri-o.arm64.v1.20.9.tar.gz)

## Changelog since v1.20.8

### Changes by Kind

#### Feature
 - Added `container_runtime_crio_containers_oom_total` and  `container_runtime_crio_containers_oom` metrics,
  which collects out of memory (oom) containers. ([#5706](https://github.com/cri-o/cri-o/pull/5706), [@haircommander](https://github.com/haircommander))

#### Bug or Regression
 - Fix a bug where ExecSync requests (exec probes) could use an arbitrary amount of memory and disk. Output from ExecSync requests is now limited to 16MB (the amount that exec output was limited to in the dockershim). Disk limiting requires conmon 2.1.2 to work. See https://github.com/cri-o/cri-o/security/advisories/GHSA-fcm2-6c3h-pg6j and CVE-2022-1708 for more information. ([#5951](https://github.com/cri-o/cri-o/pull/5951), [@haircommander](https://github.com/haircommander))

## Dependencies

### Added
_Nothing has changed._

### Changed
- github.com/json-iterator/go: [v1.1.10 → v1.1.12](https://github.com/json-iterator/go/compare/v1.1.10...v1.1.12)
- github.com/modern-go/reflect2: [v1.0.1 → v1.0.2](https://github.com/modern-go/reflect2/compare/v1.0.1...v1.0.2)

### Removed
_Nothing has changed._
