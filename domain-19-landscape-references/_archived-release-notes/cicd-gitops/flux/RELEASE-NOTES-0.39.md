---
title: flux v0.39 Release Notes
description: flux v0.39 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- helm
- flux
- docker
- job
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.39 Release Notes 是什么
- 如何 flux v0.39 Release Notes
trigger_keywords:
- flux
- v0.39
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- iac-basics
created: "2026-05-23"
---

# [[Flux|flux]] v0.39 Release Notes

Source: [v0.39.0](https://github.com/fluxcd/flux2/releases/tag/v0.39.0)

## Highlights

Flux v0.39.0 comes with new features and improvements. Users are encouraged to upgrade for the best experience.

Starting with this version, the Flux controllers come with [SBOMs and SLSA Provenance Attestations](https://fluxcd.io/flux/security/) embedded in their container images. 

The [Flux Terraform Provider](https://github.com/fluxcd/terraform-provider-flux) has a new resource for bootstrapping Flux, without depending on third-party Terraform providers, that allows customising the controllers at install time. Users are encouraged to migrate to this new resources and provide feedback.

The Flux CLI is now included in [Wolfi OS](https://github.com/wolfi-dev/os), the Linux (Un)distro designed for securing the software supply chain. The Chainguard team and Wolfi maintainers are shipping updates for the Flux package on a regular basis.

### Features and improvements

- Recreate immutable resources (e.g. Kubernetes Jobs) by annotating or labeling them with `kustomize.toolkit.fluxcd.io/force: enabled`.
- Support for HTTPS bearer token authentication for Git repositories.
- Improve memory usage by disabling the caching of Secret and ConfigMap resources in all controllers.
- Better observability with progressive status updates for Sources (Git, OCI, Helm, S3 Buckets). 
- Allow extracting the OCI artifact SHA256 digest for Cosign with `flux push artifact -o json`.
- Track CRDs managed by Flux, `flux trace` and `flux tree` will show which HelmRelease deployed which CRDs.
- Allow the Flux GitHub Action to use a GitHub token when checking for updates to avoid rate limiting.

### New documentation

- Security: [Software Bill of Materials](https://fluxcd.io/flux/security/#software-bill-of-materials)
- Security: [SLSA Provenance Attestations](https://fluxcd.io/flux/security/#slsa-provenance-attestations)
- Security: [Scanning Flux images for CVEs](https://fluxcd.io/flux/security/#scanning-for-cves)

## Components changelog

- source-controller [v0.34.0](https://github.com/fluxcd/source-controller/blob/v0.34.0/CHANGELOG.md)
- kustomize-controller [v0.33.0](https://github.com/fluxcd/kustomize-controller/blob/v0.33.0/CHANGELOG.md)
- helm-controller [v0.29.0](https://github.com/fluxcd/helm-controller/blob/v0.29.0/CHANGELOG.md)
- notification-controller [v0.31.0](https://github.com/fluxcd/notification-controller/blob/v0.31.0/CHANGELOG.md)
- image-reflector-controller [v0.24.0](https://github.com/fluxcd/image-reflector-controller/blob/v0.24.0/CHANGELOG.md)
- image-automation-controller [v0.29.0](https://github.com/fluxcd/image-automation-controller/blob/v0.29.0/CHANGELOG.md)

## CLI Changelog

- PR #3550 - @stefanprodan - flux tree: Set CRDs GroupKind in output
- PR #3549 - @stefanprodan - flux tree: Track CRDs managed by HelmReleases
- PR #3545 - @fluxcdbot - Update toolkit components
- PR #3542 - @stefanprodan - flux tree: Add namespaces to objects reconciled from HRs
- PR #3540 - @stefanprodan - Add json/yaml output to flux push artifact
- PR #3537 - @stefanprodan - Update dependencies to Kubernetes v1.26.1
- PR #3532 - @stefanprodan - Update Alpine to v3.17 and kubectl to v1.26.1 in flux-cli image
- PR #3531 - @makkes - fix misleading messaging when using `-A` flag
- PR #3529 - @dependabot[bot] - build(deps): bump docker/setup-buildx-action from 2.2.1 to 2.4.0
- PR #3526 - @dependabot[bot] - Bump anchore/sbom-action from 0.13.1 to 0.13.3
- PR #3525 - @dependabot[bot] - Bump github/codeql-action from 2.1.38 to 2.2.1
- PR #3524 - @dependabot[bot] - Bump goreleaser/goreleaser-action from 4.1.0 to 4.1.1
- PR #3517 - @jooooel - Fix broken GitHub Action and handle case where VERSION is provided as an input
- PR #3507 - @thezanke - Update prometheus-community helm repo due to the suspension of OCI builds
- PR #3501 - @kingdonb - Add GITHUB_TOKEN  to Flux GitHub Action
- PR #3488 - @dependabot[bot] - Bump snyk/actions from 1cc9026f51d822442cb4b872d8d7ead8cc69a018 to e25b2e6f5658d1bb7a6671b113260f13134cc3af
- PR #3487 - @dependabot[bot] - Bump actions/cache from 3.2.2 to 3.2.3
- PR #3486 - @dependabot[bot] - Bump github/codeql-action from 2.1.37 to 2.1.38
- PR #3477 - @raffis - fix(install-script): support $GITHUB_TOKEN

