---
title: flux v0.31 Release Notes
description: flux v0.31 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
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
- flux v0.31 Release Notes 是什么
- 如何 flux v0.31 Release Notes
trigger_keywords:
- flux
- v0.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Flux|flux]] v0.31 Release Notes

Source: [v0.31.5](https://github.com/fluxcd/flux2/releases/tag/v0.31.5)

# Highlights

Flux v0.31.5 is a patch release that comes with fixes. Users are encouraged to upgrade for the best experience.

## Fixes

- Fix ImageRepository public repository scan for unconfigured provider registries

## Improvements

- Improve [[Helm|Helm]] OCI Chart to work with registries that don't support listing tags

## Component changelog

- source-controller [v0.25.11](https://github.com/fluxcd/source-controller/blob/v0.25.11/CHANGELOG.md)
- image-reflector-controller [v0.19.4](https://github.com/fluxcd/image-reflector-controller/blob/v0.19.4/CHANGELOG.md)

## CLI Changelog
- PR #2932 - @fluxcdbot - Update toolkit components
- PR #2917 - @morancj - SRCINFO: fix path

