---
title: flux v0.19 Release Notes
description: flux v0.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.19 Release Notes 是什么
- 如何 flux v0.19 Release Notes
trigger_keywords:
- flux
- v0.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# flux v0.19 Release Notes

Source: [v0.19.1](https://github.com/fluxcd/flux2/releases/tag/v0.19.1)

If you are upgrading from 0.17 or older versions, please see the [Upgrade Flux to the v1beta2 API](https://github.com/fluxcd/flux2/discussions/1916) guide.

CHANGELOG
- PR #1996 - @hiddeco - e2e/azure: update dependencies
- PR #1993 - @fluxcdbot - Update toolkit components
- PR #1985 - @makkes - Add Max Jonas Werner to maintainer list
- PR #1984 - @stefanprodan - Fix bootstrap path check
- PR #1983 - @SomtochiAma - Add unit tests for create secret export
- PR #1982 - @stefanprodan - Add poll interval flag to flux check cmd
- PR #1978 - @darkowlzz - Minor improvements in the release procedure docs
- PR #1977 - @stefanprodan - e2e: Add test for libgit2 tag semver range
- PR #1976 - @stefanprodan - Install envtest before running the unit tests
- PR #1975 - @johngmyers - Fix inadequate quoting of KUBEBUILDER_ASSETS
- PR #1970 - @phillebaba - Fix infrastructure clean up on Azure e2e test failure


## Docker images

- `docker pull fluxcd/flux-cli:v0.19.1`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.19.1`
