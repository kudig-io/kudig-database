---
title: trivy v0.5 Release Notes
description: trivy v0.5 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.5 Release Notes 是什么
- 如何 trivy v0.5 Release Notes
trigger_keywords:
- trivy
- v0.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Trivy|trivy]] v0.5 Release Notes

Source: [v0.5.4](https://github.com/aquasecurity/trivy/releases/tag/v0.5.4)

## Bug fixes
Crash following interrupted DB download (#288)

## Changelog

e5ff5ec Fix CircleCI example in README.md (#451)
1bc02f9 fix(db): retry downloading the database if it is broken (#452)
05fa779 chore(release): add all supported versions (#445)


## Docker images

- `docker pull docker.io/aquasec/trivy:0.5.4`
- `docker pull docker.io/aquasec/trivy:latest`
