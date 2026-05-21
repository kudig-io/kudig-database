---
title: trivy v0.19 Release Notes
description: trivy v0.19 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.19 Release Notes 是什么
- 如何 trivy v0.19 Release Notes
trigger_keywords:
- trivy
- v0.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# trivy v0.19 Release Notes

Source: [v0.19.2](https://github.com/aquasecurity/trivy/releases/tag/v0.19.2)

## Changelog

f3f3029 Updated the Alpine Image to 3.14 (latest) (#1130)
0e52fde Added EOL for Ubuntu 21.10 (#1131)
9b3fba0 fix(image): disabled scanning of config files within container images (#1133)
1101634 docs: fixed typo (#1124)
499b7a6 update cyclonedx github action to v0.3.0 (#1127)


## Docker images

- `docker pull aquasec/trivy:0.19.2`
- `docker pull ghcr.io/aquasecurity/trivy:0.19.2`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.19.2`
- `docker pull aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
- `docker pull public.ecr.aws/aquasecurity/trivy:latest`
