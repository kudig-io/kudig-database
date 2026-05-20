---
title: kind v0.6 Release Notes
description: kind v0.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.6 Release Notes 是什么
- 如何 kind v0.6 Release Notes
trigger_keywords:
- kind
- v0.6
- Release
- Notes
- release
- notes
---

# kind v0.6 Release Notes

Source: [v0.6.1](https://github.com/kubernetes-sigs/kind/releases/tag/v0.6.1)

This is a small patch release over [v0.6.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.6.0).

Notably:
- Relaxed `protocol` and `propagation` in v1alpha4 config to have defaulting like v1alpha3 without specifying a value.
- Fixed containerd config patching on clusters with multiple control plane nodes.
- Fixed containerd config patching on slow hosts (do not attempt to restart containerd if it has not started yet).
- Fixed airgapped support for node images (corrected kindnetd image preloading).

This last point brings a new list of node images, fully compatible with `v0.6.0`, but with `kindnetd` properly pre-loaded onto them for use in air-gapped / offline environments:

- `kindest/node:v1.16.3@sha256:70ce6ce09bee5c34ab14aec2b84d6edb260473a60638b1b095470a3a0f95ebec`
- `kindest/node:v1.15.6@sha256:18c4ab6b61c991c249d29df778e651f443ac4bcd4e6bdd37e0c83c0d33eaae78`
- `kindest/node:v1.14.9@sha256:bdd3731588fa3ce8f66c7c22f25351362428964b6bca13048659f68b9e665b72`
- `kindest/node:v1.13.12@sha256:1fe072c080ee129a2a440956a65925ab3bbd1227cf154e2ade145b8e59a584ad `
- `kindest/node:v1.12.10@sha256:c5aeca1433e3230e6c1a96b5e1cd79c90139fd80242189b370a3248a05d77118`
- `kindest/node:v1.11.10@sha256:8ebe805201da0a988ee9bbcc2de2ac0031f9264ac24cf2a598774f1e7b324fe1 `


The default node image is now `kindest/node:v1.16.3@sha256:70ce6ce09bee5c34ab14aec2b84d6edb260473a60638b1b095470a3a0f95ebec`