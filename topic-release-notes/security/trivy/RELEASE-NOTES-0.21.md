---
title: trivy v0.21 Release Notes
description: trivy v0.21 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.21 Release Notes 是什么
- 如何 trivy v0.21 Release Notes
trigger_keywords:
- trivy
- v0.21
- Release
- Notes
- release
- notes
---

# trivy v0.21 Release Notes

Source: [v0.21.3](https://github.com/aquasecurity/trivy/releases/tag/v0.21.3)

## Changelog

8e57dee8 fix(docs): typo (#1488)
8bfbc84a feat(plugin): Add option to update plugin (#1462)
1e811de2 fix: fixed skipFiles/skipDirs flags for relative path (#1482)
8b5796f7 feat (plugin): add list and info command for plugin (#1452)
a2199bb4 fix: set up a vulnerability severity (#1458)
279e76f7 chore: add arm64 deb package (#1480)
52625908 Link to trivy tutorial on Semaphore (#1449)
c275a841 refactor(helm): externalize env vars to configMap (#1345)


## Docker images

- `docker pull aquasec/trivy:0.21.3`
- `docker pull ghcr.io/aquasecurity/trivy:0.21.3`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.21.3`
