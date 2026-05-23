---
title: flux v2.4 Release Notes
description: flux v2.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- minio
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- flux v2.4 Release Notes 是什么
- 如何 flux v2.4 Release Notes
trigger_keywords:
- flux
- v2.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
created: "2026-05-23"
---

# [[Flux|flux]] v2.4 Release Notes

Source: [v2.4.0](https://github.com/fluxcd/flux2/releases/tag/v2.4.0)

## Highlights

Flux v2.4.0 is a feature release. Users are encouraged to upgrade for the best experience.

For a comprehensive overview of new features and API changes included in this release, please refer to the [Announcing Flux 2.4 GA blog post](https://fluxcd.io/blog/2024/09/flux-v2.4.0/).

This release marks the General Availability (GA) of Flux Bucket API. The `Bucket` v1 API comes with new features including: proxy support, mTLS and custom STS configuration for AWS S3 and MinIO LDAP authentication.

The `GitRepository` v1 API gains support for OIDC authentication. Starting with this version, you can authenticate against Azure DevOps repositories using AKS Workload Identity.

The `OCIRepository` v1beta2 API gains support for proxy configuration thus allowing dedicated HTTP/S Proxy authentication on multi-tenant [[Kubernetes|Kubernetes]] clusters.

The `HelmRelease` v2 API gains support for disabling JSON schema validation of the [[Helm|Helm]] release values during installation and upgrade. And allows adopting existing Kubernetes resources during Helm release installation.

The Flux controllers are now built with Go 1.23 and their dependencies have been updated to Kubernetes 1.31, Helm 3.16, SOPS 3.9 Cosign 2.4 and Notation 1.2.

❤️ Big thanks to all the Flux contributors that helped us with this release!

### Kubernetes compatibility

This release is compatible with the following Kubernetes versions:

| Kubernetes version | Minimum required |
|--------------------|------------------|
| `v1.29`            | `>= 1.29.0`      |
| `v1.30`            | `>= 1.30.0`      |
| `v1.31`            | `>= 1.31.0`      |

> [!NOTE]
> Note that the Flux project offers support only for the latest three minor versions of Kubernetes.
> Backwards compatibility with older versions of Kubernetes and OpenShift is offered by vendors such as
> [ControlPlane](https://control-plane.io/enterprise-for-flux-cd/) that provide enterprise support for Flux.

### OpenShift compatibility

Flux can be installed on Red Hat OpenShift cluster directly from OperatorHub using [Flux Operator](https://operatorhub.io/operator/flux-operator). 
The operator allows the configuration of Flux multi-tenancy lockdown, network policies, persistent storage, sharding, vertical scaling and the synchronization of the cluster state from Git repositories, OCI artifacts and S3-compatible storage.

## API changes

### Bucket v1

The [Bucket](https://fluxcd.io/flux/components/source/buckets/) kind was promoted from v1beta2 to v1 (GA).

The v1 API is backwards compatible with v1beta2.

New fields:

- `.spec.proxySecretRef` allows configuring HTTP/S Proxy authentication for the S3-compatible storage service.
- `.spec.certSecretRef` allows custom TLS client certificate and CA for secure communication with the S3-compatible storage service.
- `.spec.sts` allows custom STS configuration for AWS S3 and MinIO LDAP authentication.

### GitRepository v1

The [GitRepository](https://fluxcd.io/flux/components/source/gitrepositoies/) kind gains new optional fields with no breaking changes.

New fields:

- `.spec.provider` allows specifying an OIDC provider used for authentication purposes. Currently, only the `azure` provider is supported.

### OCIRepository v1beta2

The [OCIRepository](https://fluxcd.io/flux/components/source/ocirepositoies/) kind gains new optional fields with no breaking changes.

New fields:

- `.spec.proxySecretRef` allows configuring HTTP/S Proxy authentication for the container registry service.

### HelmRelease v2

The [HelmRelease](https://fluxcd.io/flux/components/helm/helmreleases/) kind gains new optional fields with no breaking changes.

New fields:

- `.spec.install.disableSchemaValidation` allows  disabling the JSON schema validation of the Helm release values during installation.
- `.spec.upgrade.disableSchemaValidation` allows  disabling the JSON schema validation of the Helm release values during upgrade.

## Upgrade procedure

Upgrade Flux from `v2.3.0` to `v2.4.0` either by [rerunning bootstrap](https://fluxcd.io/flux/installation/#bootstrap-upgrade) or by using the [Flux GitHub Action](https://github.com/fluxcd/flux2/tree/main/action).

To upgrade the APIs, make sure the new CRDs and controllers are deployed, and then change the manifests in Git:

1. Set  `apiVersion: source.toolkit.fluxcd.io/v1` in the YAML files that contain `Bucket` definitions.
2. Commit, push and reconcile the API version changes.

Bumping the APIs version in manifests can be done gradually.
It is advised to not delay this procedure as the deprecated versions will be removed after 6 months.

## Components changelog

- source-controller [v1.4.0](https://github.com/fluxcd/source-controller/blob/v1.4.0/CHANGELOG.md) [v1.4.1](https://github.com/fluxcd/source-controller/blob/v1.4.1/CHANGELOG.md)
- kustomize-controller [v1.4.0](https://github.com/fluxcd/kustomize-controller/blob/v1.4.0/CHANGELOG.md)
- notification-controller [v1.4.0](https://github.com/fluxcd/notification-controller/blob/v1.4.0/CHANGELOG.md)
- helm-controller [v1.1.0](https://github.com/fluxcd/helm-controller/blob/v1.1.0/CHANGELOG.md)
- image-reflector-controller [v0.33.0](https://github.com/fluxcd/image-reflector-controller/blob/v0.33.0/CHANGELOG.md)
- image-automation-controller [v0.39.0](https://github.com/fluxcd/image-automation-controller/blob/v0.39.0/CHANGELOG.md)

### New Documentation

- [Bucket v1 specification](https://fluxcd.io/flux/components/source/buckets/)
- [Azure DevOps OIDC auth configuration](https://fluxcd.io/flux/components/source/gitrepositories/#provider)

## CLI Changelog

- PR #5014 - @stefanprodan - Update Kubernetes dependencies to v1.31.1
- PR #5011 - @stefanprodan - Remove TLS deprecated flags from `flux create secret`
- PR #5010 - @stefanprodan - Add `flux create secret proxy` command
- PR #5009 - @stefanprodan - Add `--proxy-secret-ref` to `flux create source` commands
- PR #5008 - @stefanprodan - Promote `bucket` commands to GA
- PR #5007 - @stefanprodan - Run conformance tests for Kubernetes 1.29-1.31
- PR #5005 - @fluxcdbot - Update toolkit components
- PR #5004 - @fluxcdbot - Update source-controller to v1.4.1
- PR #4986 - @dipti-pai - [RFC-0007] Add `--provider` flag to `flux create source git`
- PR #4970 - @JasonTheDeveloper - Update notaryproject/notation-go to 1.2.1
- PR #4967 - @mxtw - tests: use tempdir to avoid manual gc
- PR #4959 - @stefanprodan - Fix GitHub bootstrap for repositories with custom properties 
- PR #4948 - @harshitasao - fix: fixed GHA token-permission and pinned dependencies issue
- PR #4939 - @bkreitch - Recursively diff Kustomizations
- PR #4936 - @stefanprodan - Build with Go 1.23
- PR #4934 - @stefanprodan - Update dependencies to Kubernetes v1.31.0
- PR #4922 - @bkreitch - Stop spinner on cancel of flux diff kustomization
- PR #4918 - @matheuscscp - Fix reconcile helmrelease command description
- PR #4892 - @stefanprodan - Run conformance tests for Kubernetes v1.31
- PR #4871 - @harshitasao - changed the scorecard badge link to the standard format
- PR #4866 - @nagyv - Introduce visibility flag for bootstrap gitlab
- PR #4863 - @stefanprodan - Update conformance tests to Kubernetes v1.30.2
- PR #4845 - @stefanprodan - Run ARM64 e2e tests on GitHub runners
- PR #4842 - @stefanprodan - Add `part-of` label to controllers base
- PR #4835 - @stefanprodan - ci: Adapt config to GoRelease v2
- PR #4806 - @dipti-pai - [RFC] Passwordless authentication for Git repositories
