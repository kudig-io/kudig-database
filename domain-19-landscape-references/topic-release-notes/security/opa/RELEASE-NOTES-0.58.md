---
title: opa v0.58 Release Notes
description: opa v0.58 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- containerd
- docker
- opa
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.58 Release Notes 是什么
- 如何 opa v0.58 Release Notes
trigger_keywords:
- opa
- v0.58
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
- observability-basics
---

# opa v0.58 Release Notes

Source: [v0.58.0](https://github.com/open-policy-agent/opa/releases/tag/v0.58.0)

> **_NOTES:_**
>
> * All published OPA images now run with a non-root uid/gid. The `uid:gid` is set to `1000:1000` for all images. As a result
    there is no longer a need for the `-rootless` image variant and hence it will not be published as part of future releases.
    This change is in line with container security best practices. OPA can still be run with root privileges by explicitly setting the user,
    either with the `--user` argument for `docker run`, or by specifying the `securityContext` in the Kubernetes Pod specification.

This release contains a mix of performance improvements, bugfixes and security fixes for third-party libraries.

### Runtime, Tooling, SDK
- cmd/test: Display lines not covered if code coverage threshold not met in verbose reporting mode ([#2562](https://github.com/open-policy-agent/opa/issues/2562)) authored by @johanfylling
- cmd/test: Don't round up test coverage calculation as it could lead to inaccurate code coverage results ([#6307](https://github.com/open-policy-agent/opa/issues/6307)) authored by @anderseknert
- cmd/fmt: Don't format functions without a value to include `= true` as it is implied ([#6323](https://github.com/open-policy-agent/opa/pull/6323)) authored by @anderseknert
- server: Remove deprecated partial query parameter from REST API. This option has been deprecated since `v0.23.0` ([#2266](https://github.com/open-policy-agent/opa/issues/2266)) authored by @ashutosh-narkar
- Add support for configurable prometheus buckets for the `http_request_duration_seconds` metric ([#6238](https://github.com/open-policy-agent/opa/issues/6238)) authored by @AdrianArnautu
- plugins/bundle: Update bundle plugin state on a reconfigure operation when existing bundle is not modified ([#6311](https://github.com/open-policy-agent/opa/pull/6311)) authored by @asadk12
- internal/pathwatcher: Fix how paths to watch by a fsnotify watcher are determined to avoid monitoring unintended directories and files ([#6277](https://github.com/open-policy-agent/opa/pull/6277)) authored by @ashutosh-narkar

### Topdown and Rego
- topdown: Fix issue with build optimization producing support modules with forbidden characters in first var of rule ref ([#6338](https://github.com/open-policy-agent/opa/issues/6338)) authored by @johanfylling
- topdown: Fix panic in build optimization when policy contains rules with a general ref in the head ([#6339](https://github.com/open-policy-agent/opa/issues/6339)) authored by @johanfylling
- topdown: Avoid unnecessary conversion of small numbers by caching them and thereby helping to speed up some arithmetic operations ([#6021](https://github.com/open-policy-agent/opa/issues/6021)) authored by @ashutosh-narkar
- ast+rego: Disable compiler stages for IR-based eval paths ([#6335](https://github.com/open-policy-agent/opa/pull/6335)) authored by @srenatus
- built-in/walk: Skip path creation if path is assigned a wildcard to achieve faster `walk`-ing ([#6267](https://github.com/open-policy-agent/opa/pull/6267)) authored by @anderseknert
- ast: Add regression test for edge case where partial rule hides recursion cycle ([#6318](https://github.com/open-policy-agent/opa/pull/6318)) authored by @johanfylling

### Docs
- Drop EXPERIMENTAL status of reported prom metrics ([#6298](https://github.com/open-policy-agent/opa/issues/6298)) authored by @ashutosh-narkar
- Update documentation on GCS bundles for case where the resource (the object in the GCS bucket) contains slashes (`/`) or other special characters ([#6264](https://github.com/open-policy-agent/opa/pull/6264)) authored by @dennisg
- Provide a more clear description of negation in the policy language section ([#6275](https://github.com/open-policy-agent/opa/pull/6275)) authored by @gusega

### Website + Ecosystem
- Fix un-versioned built-in docs issue so that only the built-ins for a given doc version are displayed ([#6269](https://github.com/open-policy-agent/opa/issues/6269)) authored by @charlieegan3

### Miscellaneous
- ci: Remove `hub` tool in GitHub workflows in favor of [GitHub CLI](https://cli.github.com/) tool ([#6326](https://github.com/open-policy-agent/opa/issues/6326)) authored by @ashutosh-narkar
- Dependency updates; notably:
  - bump go.opentelemetry.io modules ([#6292](https://github.com/open-policy-agent/opa/issues/6292)) authored by @cksidharthan
  - aquasecurity/trivy-action from 0.12.0 to 0.13.0
  - github.com/containerd/containerd from 1.7.6 to 1.7.7
  - github.com/fsnotify/fsnotify from 1.6.0 to 1.7.0
  - golang.org/x/net from 0.15.0 to 0.17.0
  - google.golang.org/grpc from 1.58.2 to 1.59.0 (addresses vulnerability [GHSA-m425-mq94-257g](https://github.com/advisories/GHSA-m425-mq94-257g))
  - oras.land/oras-go/v2 from 2.3.0 to 2.3.1
  - sigs.k8s.io/yaml from 1.3.0 to 1.4.0

