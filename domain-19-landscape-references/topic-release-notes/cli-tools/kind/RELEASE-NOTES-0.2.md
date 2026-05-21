---
title: kind v0.2 Release Notes
description: kind v0.2 Release Notes — Kubernetes 生产运维知识库
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
- kind v0.2 Release Notes 是什么
- 如何 kind v0.2 Release Notes
trigger_keywords:
- kind
- v0.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# kind v0.2 Release Notes

Source: [0.2.1](https://github.com/kubernetes-sigs/kind/releases/tag/0.2.1)

0.2.1 is a bug fix release

# Breaking Changes

NONE

# New Features

-  The hostpath provisioner is now enabled by default (#397)

# Fixes

- fix `kind build node-image` on macOS, previously `--type=bazel` and `--type=docker` (the default) did not work properly on not-Linux in `0.2.0` (#413)
- fix possible panic in failed `kind create cluster` calls, previously if multiple nodes failed to come up kind could panic (#407)


<h1 id="contributors">Contributors</h1>

Thanks to everyone who committed to this release! ❤️

- @BenTheElder 
- @joejulian
- @k8s-ci-robot 
- @neolit123
- @akutz 

