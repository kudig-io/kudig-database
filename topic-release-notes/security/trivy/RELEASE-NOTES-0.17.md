---
title: trivy v0.17 Release Notes
description: trivy v0.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.17 Release Notes 是什么
- 如何 trivy v0.17 Release Notes
trigger_keywords:
- trivy
- v0.17
- Release
- Notes
- release
- notes
---

# trivy v0.17 Release Notes

Source: [v0.17.2](https://github.com/aquasecurity/trivy/releases/tag/v0.17.2)

## Changelog

415e1d8 fix: scan only regular files (#976)
3bb8852 docs: mention upx binaries (#974)
c0fddd9 chore: upgrade alpine to fix git and libcurl vulnerabilities in trivy docker image scan (#971)


## Docker images

- `docker pull aquasec/trivy:0.17.2`
- `docker pull ghcr.io/aquasecurity/trivy:0.17.2`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.17.2`
- `docker pull aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
- `docker pull public.ecr.aws/aquasecurity/trivy:latest`
