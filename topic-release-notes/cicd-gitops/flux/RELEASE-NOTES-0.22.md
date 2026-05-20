---
title: flux v0.22 Release Notes
description: flux v0.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.22 Release Notes 是什么
- 如何 flux v0.22 Release Notes
trigger_keywords:
- flux
- v0.22
- Release
- Notes
- release
- notes
---

# flux v0.22 Release Notes

Source: [v0.22.1](https://github.com/fluxcd/flux2/releases/tag/v0.22.1)

## Components changelog
- [helm-controller v0.12.2](https://github.com/fluxcd/helm-controller/blob/v0.12.2/CHANGELOG.md)
- [image-reflector-controller v0.13.1](https://github.com/fluxcd/image-reflector-controller/blob/v0.13.1/CHANGELOG.md)
- [image-automation-controller to v0.17.1](https://github.com/fluxcd/image-automation-controller/blob/v0.17.1/CHANGELOG.md)

## CLI changelog
- PR #2076 - @fluxcdbot - Update toolkit components
- PR #2075 - @jack-evans - Remove trailing `---` for `flux install` to match `flux bootstrap` generated YAML


## Docker images

- `docker pull fluxcd/flux-cli:v0.22.1`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.22.1`
