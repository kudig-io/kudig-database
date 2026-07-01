---
title: cri-o v1.31 Release Notes
description: cri-o v1.31 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.31 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.31 Release Notes 是什么
- 如何 cri-o v1.31 Release Notes
trigger_keywords:
- cri-o
- v1.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# cri-o v1.31 Release Notes

Source: [v1.31.13](https://github.com/cri-o/cri-o/releases/tag/v1.31.13)

- [CRI-O v1.31.13](#cri-o-v13113)
  - [Downloads](#downloads)
  - Changelog since v1.31.12](#changelog-since-v13112)
    - [Changes by Kind](#changes-by-kind)
      - [Bug or Regression](#bug-or-regression)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.31.13

The release notes have been generated for the commit range
[v1.31.12...v1.31.13](https://github.com/cri-o/cri-o/compare/v1.31.12...v1.31.13) on Thu, 02 Oct 2025 00:22:29 UTC.

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.v1.31.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz)
  - [cri-o.amd64.v1.31.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz.sha256sum)
  - [cri-o.amd64.v1.31.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz.sig)
  - [cri-o.amd64.v1.31.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz.cert)
  - [cri-o.amd64.v1.31.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz.spdx)
  - [cri-o.amd64.v1.31.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz.spdx.sig)
  - [cri-o.amd64.v1.31.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.31.13.tar.gz.spdx.cert)
- [cri-o.arm64.v1.31.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz)
  - [cri-o.arm64.v1.31.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz.sha256sum)
  - [cri-o.arm64.v1.31.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz.sig)
  - [cri-o.arm64.v1.31.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz.cert)
  - [cri-o.arm64.v1.31.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz.spdx)
  - [cri-o.arm64.v1.31.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz.spdx.sig)
  - [cri-o.arm64.v1.31.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.31.13.tar.gz.spdx.cert)
- [cri-o.ppc64le.v1.31.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz)
  - [cri-o.ppc64le.v1.31.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz.sha256sum)
  - [cri-o.ppc64le.v1.31.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz.sig)
  - [cri-o.ppc64le.v1.31.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz.cert)
  - [cri-o.ppc64le.v1.31.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz.spdx)
  - [cri-o.ppc64le.v1.31.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz.spdx.sig)
  - [cri-o.ppc64le.v1.31.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.31.13.tar.gz.spdx.cert)
- [cri-o.s390x.v1.31.13.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz)
  - [cri-o.s390x.v1.31.13.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz.sha256sum)
  - [cri-o.s390x.v1.31.13.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz.sig)
  - [cri-o.s390x.v1.31.13.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz.cert)
  - [cri-o.s390x.v1.31.13.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz.spdx)
  - [cri-o.s390x.v1.31.13.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz.spdx.sig)
  - [cri-o.s390x.v1.31.13.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.31.13.tar.gz.spdx.cert)

To verify the artifact signatures via [cosign](https://github.com/sigstore/cosign), run:

```console
> export COSIGN_EXPERIMENTAL=1
> cosign verify-blob cri-o.amd64.v1.31.13.tar.gz \
    --certificate-identity https://github.com/cri-o/cri-o/.github/workflows/test.yml@refs/tags/v1.31.13 \
    --certificate-oidc-issuer https://token.actions.githubusercontent.com \
    --certificate-github-workflow-repository cri-o/cri-o \
    --certificate-github-workflow-ref refs/tags/v1.31.13 \
    --signature cri-o.amd64.v1.31.13.tar.gz.sig \
    --certificate cri-o.amd64.v1.31.13.tar.gz.cert
```

To verify the bill of materials (SBOM) in [SPDX](https://spdx.org) format using the [bom](https://sigs.k8s.io/bom) tool, run:

```console
> tar xfz cri-o.amd64.v1.31.13.tar.gz
> bom validate -e cri-o.amd64.v1.31.13.tar.gz.spdx -d cri-o
```

## Changelog since v1.31.12

### Changes by Kind

#### Bug or Regression
 - Fix log rotation not working for containers running with the kata-containers runtime (#9450, @littlejawa)

## Dependencies

### Added
_Nothing has changed._

### Changed
_Nothing has changed._

### Removed
_Nothing has changed._
