---
title: cri-o v1.30 Release Notes
description: cri-o v1.30 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v1.30 Release Notes 是什么
- 如何 cri-o v1.30 Release Notes
trigger_keywords:
- cri-o
- v1.30
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cri-o v1.30 Release Notes

Source: v1.30.14](https://github.com/cri-o/cri-o/releases/tag/v1.30.14)

- [CRI-O v1.30.14](#cri-o-v13014)
  - [Downloads](#downloads)
  - Changelog since v1.30.13](#changelog-since-v13013)
    - [Changes by Kind](#changes-by-kind)
      - [Uncategorized](#uncategorized)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.30.14

The release notes have been generated for the commit range
[v1.30.13...v1.30.14](https://github.com/cri-o/cri-o/compare/v1.30.13...v1.30.14) on Wed, 04 Jun 2025 00:34:05 UTC.

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.v1.30.14.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz)
  - [cri-o.amd64.v1.30.14.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz.sha256sum)
  - [cri-o.amd64.v1.30.14.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz.sig)
  - [cri-o.amd64.v1.30.14.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz.cert)
  - [cri-o.amd64.v1.30.14.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz.spdx)
  - [cri-o.amd64.v1.30.14.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz.spdx.sig)
  - [cri-o.amd64.v1.30.14.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.30.14.tar.gz.spdx.cert)
- [cri-o.arm64.v1.30.14.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz)
  - [cri-o.arm64.v1.30.14.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz.sha256sum)
  - [cri-o.arm64.v1.30.14.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz.sig)
  - [cri-o.arm64.v1.30.14.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz.cert)
  - [cri-o.arm64.v1.30.14.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz.spdx)
  - [cri-o.arm64.v1.30.14.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz.spdx.sig)
  - [cri-o.arm64.v1.30.14.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.30.14.tar.gz.spdx.cert)
- [cri-o.ppc64le.v1.30.14.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz)
  - [cri-o.ppc64le.v1.30.14.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz.sha256sum)
  - [cri-o.ppc64le.v1.30.14.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz.sig)
  - [cri-o.ppc64le.v1.30.14.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz.cert)
  - [cri-o.ppc64le.v1.30.14.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz.spdx)
  - [cri-o.ppc64le.v1.30.14.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz.spdx.sig)
  - [cri-o.ppc64le.v1.30.14.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.30.14.tar.gz.spdx.cert)
- [cri-o.s390x.v1.30.14.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz)
  - [cri-o.s390x.v1.30.14.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz.sha256sum)
  - [cri-o.s390x.v1.30.14.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz.sig)
  - [cri-o.s390x.v1.30.14.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz.cert)
  - [cri-o.s390x.v1.30.14.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz.spdx)
  - [cri-o.s390x.v1.30.14.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz.spdx.sig)
  - [cri-o.s390x.v1.30.14.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.30.14.tar.gz.spdx.cert)

To verify the artifact signatures via [cosign](https://github.com/sigstore/cosign), run:

```console
> export COSIGN_EXPERIMENTAL=1
> cosign verify-blob cri-o.amd64.v1.30.14.tar.gz \
    --certificate-identity https://github.com/cri-o/cri-o/.github/workflows/test.yml@refs/tags/v1.30.14 \
    --certificate-oidc-issuer https://token.actions.githubusercontent.com \
    --certificate-github-workflow-repository cri-o/cri-o \
    --certificate-github-workflow-ref refs/tags/v1.30.14 \
    --signature cri-o.amd64.v1.30.14.tar.gz.sig \
    --certificate cri-o.amd64.v1.30.14.tar.gz.cert
```

To verify the bill of materials (SBOM) in [SPDX](https://spdx.org) format using the [bom](https://sigs.k8s.io/bom) tool, run:

```console
> tar xfz cri-o.amd64.v1.30.14.tar.gz
> bom validate -e cri-o.amd64.v1.30.14.tar.gz.spdx -d cri-o
```

## Changelog since v1.30.13

### Changes by Kind

#### Uncategorized
 - Disabled `pull-progress-timeout` per default. (#9144, @openshift-cherrypick-robot)

## Dependencies

### Added
_Nothing has changed._

### Changed
_Nothing has changed._

### Removed
_Nothing has changed._
