---
title: flux v0.35 Release Notes
description: flux v0.35 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- helm
- flux
- gateway
- crd
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
- flux v0.35 Release Notes 是什么
- 如何 flux v0.35 Release Notes
trigger_keywords:
- flux
- v0.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- monitoring-basics
---



# [[Flux|flux]] v0.35 Release Notes

Source: [v0.35.0](https://github.com/fluxcd/flux2/releases/tag/v0.35.0)

## Highlights

Flux v0.35.0 comes with new features and improvements. Users are encouraged to upgrade for the best experience.

### Breaking changes

Strict validation rules have been put in place for API fields which define a time duration, such as `.spec.interval`. Effectively, this means values without a time unit (e.g. `ms`, `s`, `m`, `h`) will now be rejected by the API server.

### Features and improvements

- Verify OCI artifacts signed by Cosign (including keyless) with [OCIRepository.spec.verify](https://fluxcd.io/docs/components/source/ocirepositories/#verification).
- Allow pulling [[Helm|Helm]] charts dependencies from HTTPS repositories with mixed self-signed TLS and public CAs.
- Allow pulling Helm charts from OCI artifacts stored at the root of AWS ECR.
- Allow running bootstrap for insecure HTTP Git servers with `flux bootstrap git --allow-insecure-http --token-auth`.
- Improve health checking for global objects such as ClusterClass, GatewayClass, StorageClass, etc.
- The controllers and the Flux CLI are now built with Go 1.19.

For more information on OCI and Cosign support please see the [Flux documentation](https://fluxcd.io/docs/cheatsheets/oci-artifacts/#signing-and-verification).

## Components changelog

- source-controller [v0.30.0](https://github.com/fluxcd/source-controller/blob/v0.30.0/CHANGELOG.md) 
- kustomize-controller [v0.29.0](https://github.com/fluxcd/kustomize-controller/blob/v0.29.0/CHANGELOG.md)
- helm-controller [v0.25.0](https://github.com/fluxcd/helm-controller/blob/v0.25.0/CHANGELOG.md)
- notification-controller [v0.27.0](https://github.com/fluxcd/notification-controller/blob/v0.27.0/CHANGELOG.md)
- image-reflector-controller [v0.22.0](https://github.com/fluxcd/image-reflector-controller/blob/v0.22.0/CHANGELOG.md)
- image-automation-controller [v0.26.0](https://github.com/fluxcd/image-automation-controller/blob/v0.26.0/CHANGELOG.md)

## CLI Changelog

- PR #3154 - @stefanprodan - [RFC-0003] Add Cosign keyless specification
- PR #3153 - @stefanprodan - Build with Go 1.19
- PR #3149 - @fluxcdbot - Update toolkit components
- PR #3145 - @stefanprodan - Add component label for controllers and their CRDs
- PR #3117 - @carlosonunez-vmw - Maintain original scheme when using --token-auth
- PR #3098 - @Santosh1176 - [Grafana] Use `container_memory_working_set_bytes` to report memory consumption

