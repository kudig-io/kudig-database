---
title: cri-o v1.29 Release Notes
description: cri-o v1.29 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
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
- cri-o v1.29 Release Notes 是什么
- 如何 cri-o v1.29 Release Notes
trigger_keywords:
- cri-o
- v1.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cri-o v1.29 Release Notes

Source: [v1.29.13](https://github.com/cri-o/cri-o/releases/tag/v1.29.13)

- [CRI-O v1.29.13](#cri-o-v12913)
  - [Downloads](#downloads)
  - Changelog since v1.29.12](#changelog-since-v12912)
    - [Changes by Kind](#changes-by-kind)
      - [Ci](#ci)
      - [Uncategorized](#uncategorized)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.29.13

The release notes have been generated for the commit range
[v1.29.12...v1.29.13](https://github.com/cri-o/cri-o/compare/v1.29.12...v1.29.13) on Tue, 04 Feb 2025 00:21:16 UTC.

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.v1.29.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz)
  - [cri-o.amd64.v1.29.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz.sha256sum)
  - [cri-o.amd64.v1.29.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz.sig)
  - [cri-o.amd64.v1.29.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz.cert)
  - [cri-o.amd64.v1.29.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz.spdx)
  - [cri-o.amd64.v1.29.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz.spdx.sig)
  - [cri-o.amd64.v1.29.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.29.13.tar.gz.spdx.cert)
- [cri-o.arm64.v1.29.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz)
  - [cri-o.arm64.v1.29.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz.sha256sum)
  - [cri-o.arm64.v1.29.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz.sig)
  - [cri-o.arm64.v1.29.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz.cert)
  - [cri-o.arm64.v1.29.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz.spdx)
  - [cri-o.arm64.v1.29.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz.spdx.sig)
  - [cri-o.arm64.v1.29.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.29.13.tar.gz.spdx.cert)
- [cri-o.ppc64le.v1.29.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz)
  - [cri-o.ppc64le.v1.29.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz.sha256sum)
  - [cri-o.ppc64le.v1.29.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz.sig)
  - [cri-o.ppc64le.v1.29.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz.cert)
  - [cri-o.ppc64le.v1.29.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz.spdx)
  - [cri-o.ppc64le.v1.29.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz.spdx.sig)
  - [cri-o.ppc64le.v1.29.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.29.13.tar.gz.spdx.cert)
- [cri-o.s390x.v1.29.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz)
  - [cri-o.s390x.v1.29.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz.sha256sum)
  - [cri-o.s390x.v1.29.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz.sig)
  - [cri-o.s390x.v1.29.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz.cert)
  - [cri-o.s390x.v1.29.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz.spdx)
  - [cri-o.s390x.v1.29.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz.spdx.sig)
  - [cri-o.s390x.v1.29.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.29.13.tar.gz.spdx.cert)

To verify the artifact signatures via [cosign](https://github.com/sigstore/cosign), run:

```console
> export COSIGN_EXPERIMENTAL=1
> cosign verify-blob cri-o.amd64.v1.29.13.tar.gz \
    --certificate-identity https://github.com/cri-o/cri-o/.github/workflows/test.yml@refs/tags/v1.29.13 \
    --certificate-oidc-issuer https://token.actions.githubusercontent.com \
    --certificate-github-workflow-repository cri-o/cri-o \
    --certificate-github-workflow-ref refs/tags/v1.29.13 \
    --signature cri-o.amd64.v1.29.13.tar.gz.sig \
    --certificate cri-o.amd64.v1.29.13.tar.gz.cert
```

To verify the bill of materials (SBOM) in [SPDX](https://spdx.org) format using the [bom](https://sigs.k8s.io/bom) tool, run:

```console
> tar xfz cri-o.amd64.v1.29.13.tar.gz
> bom validate -e cri-o.amd64.v1.29.13.tar.gz.spdx -d cri-o
```

## Changelog since v1.29.12

### Changes by Kind

#### Ci
 - Fixed build issue with newer golang versions. (#8930, @saschagrunert)

#### Uncategorized
 - Fixed issue when sandbox removal is not possible due to stale or missing network namespace path. (#8818, @openshift-cherrypick-robot)

## Dependencies

### Added
_Nothing has changed._

### Changed
- github.com/containers/storage: [9811eb0 → 2d261ce](https://github.com/containers/storage/compare/9811eb0...2d261ce)
- github.com/cpuguy83/go-md2man/v2: [v2.0.3 → v2.0.5](https://github.com/cpuguy83/go-md2man/compare/v2.0.3...v2.0.5)
- github.com/urfave/cli: [v1.22.14 → v1.22.16](https://github.com/urfave/cli/compare/v1.22.14...v1.22.16)
- github.com/vbatts/tar-split: [v0.11.5 → v0.11.7](https://github.com/vbatts/tar-split/compare/v0.11.5...v0.11.7)
- golang.org/x/sys: v0.21.0 → v0.26.0

### Removed
_Nothing has changed._


<!-- risk-assessed -->
