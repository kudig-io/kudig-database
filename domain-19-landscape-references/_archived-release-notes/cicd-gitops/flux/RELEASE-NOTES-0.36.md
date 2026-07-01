---
title: flux v0.36 Release Notes
description: flux v0.36 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.36 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- webhook
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
- flux v0.36 Release Notes 是什么
- 如何 flux v0.36 Release Notes
trigger_keywords:
- flux
- v0.36
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- iac-basics
---



# [[Flux|flux]] v0.36 Release Notes

Source: [v0.36.0](https://github.com/fluxcd/flux2/releases/tag/v0.36.0)

## Highlights

Flux v0.36.0 comes with new features and improvements. Users are encouraged to upgrade for the best experience.

### Features and improvements

- Verify OCI [[Helm|Helm]] charts signed by Cosign (including keyless) with [HelmChart.spec.verify](https://fluxcd.io/docs/cheatsheets/oci-artifacts/#verify-helm-charts).
- Allow publishing a single YAML file to OCI with `flux push artifact <URL> --path=deploy/install.yaml`.
- Detect changes to local files before pushing to OCI with `flux diff artifact <URL> --path=<local files>`.
- New Alert Provider type named `generic-hmac` for authenticating the webhook requests coming from notification-controller.
- The `Kustomization.status.conditions` have been aligned with [[Kubernetes|Kubernetes]] standard conditions and kstatus.
- The kustomize-controller memory usage was reduced by 90% when performing artifact operations.

### New documentation

- Guide: [How to deploy Flagger with Flux using signed Helm charts and OCI artifacts](https://fluxcd.io/flagger/install/flagger-install-with-flux/)
- FAQ: [Should I be using Kustomize remote bases?](https://fluxcd.io/flux/faq/#should-i-be-using-kustomize-remote-bases)
- FAQ: [Should I be using Kustomize Helm chart plugin?](https://fluxcd.io/flux/faq/#should-i-be-using-kustomize-helm-chart-plugin)

## Components changelog

- source-controller [v0.31.0](https://github.com/fluxcd/source-controller/blob/v0.31.0/CHANGELOG.md) 
- kustomize-controller [v0.30.0](https://github.com/fluxcd/kustomize-controller/blob/v0.30.0/CHANGELOG.md)
- helm-controller [v0.26.0](https://github.com/fluxcd/helm-controller/blob/v0.26.0/CHANGELOG.md)
- notification-controller [v0.28.0](https://github.com/fluxcd/notification-controller/blob/v0.28.0/CHANGELOG.md)
- image-reflector-controller [v0.22.1](https://github.com/fluxcd/image-reflector-controller/blob/v0.22.1/CHANGELOG.md)
- image-automation-controller [v0.26.1](https://github.com/fluxcd/image-automation-controller/blob/v0.26.1/CHANGELOG.md)

## CLI Changelog

- PR #3242 - @stefanprodan - Update dependencies
- PR #3237 - @phillebaba - Move bootstrap package from internal to pkg
- PR #3236 - @stefanprodan - ci: Refactor GitHub workflows
- PR #3232 - @eddie-knight - Additional workflow permissions tweaks
- PR #3231 - @eddie-knight - Adjusted workflow permissions
- PR #3229 - @stefanprodan - RFC-0002: Add Cosign verification for Helm OCI charts
- PR #3224 - @developer-guy - Add `diff artifact` command
- PR #3220 - @stefanprodan - Only run e2e tests for Dependabot PRs
- PR #3219 - @dependabot[bot] - Bump github/codeql-action from 1 to 2
- PR #3218 - @dependabot[bot] - Bump peter-evans/create-pull-request from 3 to 4
- PR #3217 - @dependabot[bot] - Bump hashicorp/setup-terraform from 1 to 2.0.2
- PR #3216 - @stefanprodan - Enable Dependabot for GitHub Actions
- PR #3214 - @eddie-knight - Added ArtifactHub badge
- PR #3213 - @stefanprodan - Add FOSSA license scanning badge
- PR #3198 - @phillebaba - Add nop logger
- PR #3197 - @phillebaba - Move uninstall code to pkg
- PR #3190 - @developer-guy - Accept a file path as input for `flux build|push artifact`
- PR #3187 - @fluxcdbot - Update toolkit components
- PR #3174 - @phillebaba - Update libgit2 version in Azure e2e tests
- PR #3162 - @somtochiama - Update golden file for `get source oci`
- PR #3161 - @stefanprodan - Update RFC-0003 implementation history

