---
title: cri-o v1.18 Release Notes
description: cri-o v1.18 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
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
- cri-o v1.18 Release Notes 是什么
- 如何 cri-o v1.18 Release Notes
trigger_keywords:
- cri-o
- v1.18
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cri-o v1.18 Release Notes

Source: [v1.18.6](https://github.com/cri-o/cri-o/releases/tag/v1.18.6)

# CRI-O v1.18.6

The release notes have been generated for the commit range [v1.18.5...v1.18.6](https://github.com/cri-o/cri-o/compare/v1.18.5...v1.18.6).

## Downloads

Download the static release bundle via our Google Cloud Bucket: [crio-v1.18.6.tar.gz][0]

[0]: https://storage.googleapis.com/cri-o/artifacts/crio-v1.18.6.tar.gz

## Changes by Kind

### Other

- Fixed invalid version in `crio version` output

<!-- risk-assessed -->
