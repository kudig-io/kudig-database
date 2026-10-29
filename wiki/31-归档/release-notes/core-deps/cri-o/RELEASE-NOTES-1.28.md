---
title: cri-o v1.28 Release Notes
description: cri-o v1.28 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.28 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.28 Release Notes 是什么
- 如何 cri-o v1.28 Release Notes
trigger_keywords:
- cri-o
- v1.28
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




# cri-o v1.28 Release Notes

Source: [v1.28.11](https://github.com/cri-o/cri-o/releases/tag/v1.28.11)

- [CRI-O v1.28.11](#cri-o-v12811)
  - [Downloads](#downloads)
  - Changelog since v1.28.10](#changelog-since-v12810)
    - [Changes by Kind](#changes-by-kind)
      - [Uncategorized](#uncategorized)
  - [Dependencies](#dependencies)
    - [Added](#added)
    - [Changed](#changed)
    - [Removed](#removed)

# CRI-O v1.28.11

The release notes have been generated for the commit range
[v1.28.10...v1.28.11](https://github.com/cri-o/cri-o/compare/v1.28.10...v1.28.11) on Mon, 07 Oct 2024 08:35:15 UTC.

## Downloads

Download one of our static release bundles via our Google Cloud Bucket:

- [cri-o.amd64.v1.28.11.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz)
  - [cri-o.amd64.v1.28.11.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz.sha256sum)
  - [cri-o.amd64.v1.28.11.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz.sig)
  - [cri-o.amd64.v1.28.11.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz.cert)
  - [cri-o.amd64.v1.28.11.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz.spdx)
  - [cri-o.amd64.v1.28.11.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz.spdx.sig)
  - [cri-o.amd64.v1.28.11.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.amd64.v1.28.11.tar.gz.spdx.cert)
- [cri-o.arm64.v1.28.11.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz)
  - [cri-o.arm64.v1.28.11.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz.sha256sum)
  - [cri-o.arm64.v1.28.11.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz.sig)
  - [cri-o.arm64.v1.28.11.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz.cert)
  - [cri-o.arm64.v1.28.11.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz.spdx)
  - [cri-o.arm64.v1.28.11.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz.spdx.sig)
  - [cri-o.arm64.v1.28.11.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.arm64.v1.28.11.tar.gz.spdx.cert)
- [cri-o.ppc64le.v1.28.11.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz)
  - [cri-o.ppc64le.v1.28.11.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz.sha256sum)
  - [cri-o.ppc64le.v1.28.11.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz.sig)
  - [cri-o.ppc64le.v1.28.11.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz.cert)
  - [cri-o.ppc64le.v1.28.11.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz.spdx)
  - [cri-o.ppc64le.v1.28.11.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz.spdx.sig)
  - [cri-o.ppc64le.v1.28.11.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.ppc64le.v1.28.11.tar.gz.spdx.cert)
- [cri-o.s390x.v1.28.11.tar.gz](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz)
  - [cri-o.s390x.v1.28.11.tar.gz.sha256sum](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz.sha256sum)
  - [cri-o.s390x.v1.28.11.tar.gz.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz.sig)
  - [cri-o.s390x.v1.28.11.tar.gz.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz.cert)
  - [cri-o.s390x.v1.28.11.tar.gz.spdx](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz.spdx)
  - [cri-o.s390x.v1.28.11.tar.gz.spdx.sig](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz.spdx.sig)
  - [cri-o.s390x.v1.28.11.tar.gz.spdx.cert](https://storage.googleapis.com/cri-o/artifacts/cri-o.s390x.v1.28.11.tar.gz.spdx.cert)

To verify the artifact signatures via [cosign](https://github.com/sigstore/cosign), run:

```console
> export COSIGN_EXPERIMENTAL=1
> cosign verify-blob cri-o.amd64.v1.28.11.tar.gz \
    --certificate-identity https://github.com/cri-o/cri-o/.github/workflows/test.yml@refs/tags/v1.28.11 \
    --certificate-oidc-issuer https://token.actions.githubusercontent.com \
    --certificate-github-workflow-repository cri-o/cri-o \
    --certificate-github-workflow-ref refs/tags/v1.28.11 \
    --signature cri-o.amd64.v1.28.11.tar.gz.sig \
    --certificate cri-o.amd64.v1.28.11.tar.gz.cert
```

To verify the bill of materials (SBOM) in [SPDX](https://spdx.org) format using the [bom](https://sigs.k8s.io/bom) tool, run:

```console
> tar xfz cri-o.amd64.v1.28.11.tar.gz
> bom validate -e cri-o.amd64.v1.28.11.tar.gz.spdx -d cri-o
```

## Changelog since v1.28.10

### Changes by Kind

#### Uncategorized
 - Fix a bug where the GID is not added to /etc/group when run_as_group is set (#8564, @kwilczynski)
 - Fixed container stats label filtering. (#8576, @openshift-cherrypick-robot)

## Dependencies

### Added
_Nothing has changed._

### Changed
_Nothing has changed._

### Removed
_Nothing has changed._


<!-- risk-assessed -->
