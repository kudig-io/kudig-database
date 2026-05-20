---
title: flux v0.33 Release Notes
description: flux v0.33 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.33 Release Notes 是什么
- 如何 flux v0.33 Release Notes
trigger_keywords:
- flux
- v0.33
- Release
- Notes
- release
- notes
---

# flux v0.33 Release Notes

Source: [v0.33.0](https://github.com/fluxcd/flux2/releases/tag/v0.33.0)

## Highlights

Flux v0.33.0 comes with new features and improvements. Users are encouraged to upgrade for the best experience.

### Features and improvements

- [HelmRepository.spec.provider](https://fluxcd.io/docs/components/source/helmrepositories/#provider) Enable contextual login to container registries when pulling Helm charts from Amazon Elastic Container Registry, Azure Container Registry and Google Artifact Registry.
- [OCIRepository.spec.layerSelector](https://fluxcd.io/docs/components/source/ocirepositories/#layer-selector) Select which layer contains the Kubernetes configs by specifying a matching OCI media type.
- [Bucket.spec.secretRef](https://fluxcd.io/docs/components/source/buckets/#azure-blob-sas-token-example) Authenticate to Azure Blob storage using SAS tokens.
- Allow filtering OCI artifacts by semver and regex when listing artifact with `flux list artifacts`.
- Allow excluding local files and directories when building and publishing artifacts with `flux push artifact`.
- Mitigate denial-of-service on multi-tenant clusters by automatically recovering from panics encountered during reconciliation.
- Update controllers to Kubernetes v1.25.0, Kustomize v4.5.7 and Helm v3.9.4.

### New documentation

- [Secrets Management](https://fluxcd.io/docs/security/secrets-management/)
- [Contextual Authorization](https://fluxcd.io/docs/security/contextual-authorization/)

## Components changelog

- source-controller [v0.27.0](https://github.com/fluxcd/source-controller/blob/v0.27.0/CHANGELOG.md) [v0.28.0](https://github.com/fluxcd/source-controller/blob/v0.28.0/CHANGELOG.md) 
- kustomize-controller [v0.27.1](https://github.com/fluxcd/kustomize-controller/blob/v0.27.1/CHANGELOG.md)
- helm-controller [v0.23.1](https://github.com/fluxcd/helm-controller/blob/v0.23.1/CHANGELOG.md)
- notification-controller [v0.25.2](https://github.com/fluxcd/notification-controller/blob/v0.25.2/CHANGELOG.md)
- image-reflector-controller [v0.20.1](https://github.com/fluxcd/image-reflector-controller/blob/v0.20.1/CHANGELOG.md)
- image-automation-controller [v0.24.2](https://github.com/fluxcd/image-automation-controller/blob/v0.24.2/CHANGELOG.md)

## CLI Changelog

- PR #3049 - @stefanprodan - Update Kubernetes dependencies to v1.25.0
- PR #3034 - @snebel29 - Fix broken "edit this page" links in Flux CLI section
- PR #3028 - @snebel29 - Update tests/azure github.com/hashicorp/terraform-exec to v0.16.1
- PR #3025 - @stefanprodan - [RFC-0002] Add auth specification for Helm OCI
- PR #3024 - @stefanprodan - Add version validation to install commands
- PR #3019 - @somtochiama - Improve error message in get cmd
- PR #3014 - @stefanprodan - [RFC-0003] Select layer by OCI media type
- PR #2999 - @fluxcdbot - Update toolkit components
- PR #2998 - @somtochiama - Add `--filter-semver` and `--filter-regex` flags to `list artifacts`
- PR #2997 - @stefanprodan - Use ghcr.io in the static manifests
- PR #2996 - @stefanprodan - Update dependencies
- PR #2995 - @stefanprodan - Add `--ignore-paths` arg to `flux build|push artifact`
- PR #2979 - @stefanprodan - Status update for RFC-0002 and RFC-0003

