---
title: flux v0.34 Release Notes
description: flux v0.34 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.34 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- helm
- flux
- docker
- gateway
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
- flux v0.34 Release Notes 是什么
- 如何 flux v0.34 Release Notes
trigger_keywords:
- flux
- v0.34
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Flux|flux]] v0.34 Release Notes

Source: [v0.34.0](https://github.com/fluxcd/flux2/releases/tag/v0.34.0)

## Highlights

Flux v0.34.0 comes with new features and improvements. Users are encouraged to upgrade for the best experience.

### Breaking changes

The Flux controller logs have been aligned with the [[Kubernetes|Kubernetes]] structured logging.
For more details on the new logging structure please see: [fluxcd/flux2#3051](https://github.com/fluxcd/flux2/issues/3051).

### Features and improvements

- [OCIRepository.spec.insecure](https://fluxcd.io/docs/components/source/ocirepositories/#insecure) Allow pulling artifacts from an in-cluster Docker Registry over plain HTTP.
- Allow defining OCI sources for non-TLS container registries with `flux create source oci --insecure`.
- Enable contextual login when publishing OCI artifacts from a Cloud VM using `flux push artifact --provider=aws|azure|gcp`.
- Prioritise static credentials over OIDC providers when pulling OCI artifacts from container registries on multi-tenant cluster.
- Reconcile Kubernetes Class types (ClusterClass, GatewayClass, StorageClass, etc) in a dedicated stage before any other custom resources like Clusters, Gateways, Volumes, etc.
- When multiple SOPS providers are available, run the offline decryption methods first to avoid failures due to KMS unavailability. 
- Add finalizers to the notification API to properly record the reconciliation metrics for deleted resources.
- Publish the Flux install manifests as OCI artifacts on GitHub and DockerHub container registries under `fluxcd/flux-manifests`.

## Components Changelog

- source-controller [v0.29.0](https://github.com/fluxcd/source-controller/blob/v0.27.0/CHANGELOG.md) 
- kustomize-controller [v0.28.0](https://github.com/fluxcd/kustomize-controller/blob/v0.28.0/CHANGELOG.md)
- helm-controller [v0.24.0](https://github.com/fluxcd/helm-controller/blob/v0.24.0/CHANGELOG.md)
- notification-controller [v0.26.0](https://github.com/fluxcd/notification-controller/blob/v0.26.0/CHANGELOG.md)
- image-reflector-controller [v0.21.0](https://github.com/fluxcd/image-reflector-controller/blob/v0.21.0/CHANGELOG.md)
- image-automation-controller [v0.25.0](https://github.com/fluxcd/image-automation-controller/blob/v0.25.0/CHANGELOG.md)

## CLI Changelog

- PR #3097 - @stefanprodan - Add `--insecure` flag to `flux create source oci`
- PR #3091 - @fluxcdbot - Update toolkit components
- PR #3088 - @stefanprodan - Publish the install manifests to GHCR and DockerHub as OCI artifacts
- PR #3087 - @somtochiama - Remove finalizers for notification CRs on uninstall 
- PR #3085 - @souleb - [bootstrap] Make sure we reconcile with the right reconciliation method
- PR #3082 - @stefanprodan - Remove finalizers for OCI repositories on uninstall
- PR #3079 - @adrien-f - Support autologin when pushing OCI artifacts
- PR #3073 - @acondrat - Filter out non-running pods in Prometheus
- PR #3063 - @somtochiama - Update `flux logs` to accomodate the new format 
- PR #3053 - @dholbach - Revert "Fix broken "edit this page" links in Flux CLI section"
- PR #3052 - @dholbach - update to new doc links structure
- PR #3050 - @stefanprodan - Status update for RFC-0002 and RFC-0003



<!-- risk-assessed -->
