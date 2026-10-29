---
title: flux v0.23 Release Notes
description: flux v0.23 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.23 Release Notes 是什么
- 如何 flux v0.23 Release Notes
trigger_keywords:
- flux
- v0.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Flux|flux]] v0.23 Release Notes

Source: [v0.23.0](https://github.com/fluxcd/flux2/releases/tag/v0.23.0)

## Highlights

This version comes with artifact integrity verification based on SHA-2 and fixes for image automation.

## Components changelog

- [source-controller v0.18.0](https://github.com/fluxcd/source-controller/blob/v0.18.0/CHANGELOG.md)
- [kustomize-controller v0.18.0](https://github.com/fluxcd/kustomize-controller/blob/v0.18.0/CHANGELOG.md)
- [helm-controller v0.13.0](https://github.com/fluxcd/helm-controller/blob/v0.13.0/CHANGELOG.md)
- [image-reflector-controller v0.13.2](https://github.com/fluxcd/image-reflector-controller/blob/v0.13.2/CHANGELOG.md)

## CLI changelog
- PR #2080 - @fluxcdbot - Update toolkit components

## Docker images

- `docker pull fluxcd/flux-cli:v0.23.0`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.23.0`

<!-- risk-assessed -->
