---
title: trivy v0.4 Release Notes
description: trivy v0.4 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.4 Release Notes 是什么
- 如何 trivy v0.4 Release Notes
trigger_keywords:
- trivy
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Trivy|trivy]] v0.4 Release Notes

Source: [v0.4.4](https://github.com/aquasecurity/trivy/releases/tag/v0.4.4)



## Changelog

42043a0 fix(client): add image name and build time (#402)
246793e fix(redhat): use binary package name for OVAL (#393)
692b0f1 cli: append warning when --template option is ignored (#391)
0629e1d fix(cli): reject multiple images (#392)
9707c7b Initial GitLab CI template to deeply integrated with GitLab Container Scanning (#376)
194fbef feat(): include GitLab template inside the docker container (#388)
f7db00c Modify template for GitLab Container Scanning (#387)
2f4b31e chore(goreleaser): bump up to 0.124.1 (#383)
9289624 doc: Update GitLab CI example documentation (#375)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.4.4`
- `docker pull docker.io/aquasec/trivy:latest`
