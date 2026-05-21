---
title: opa v0.61 Release Notes
description: opa v0.61 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- containerd
- docker
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.61 Release Notes 是什么
- 如何 opa v0.61 Release Notes
trigger_keywords:
- opa
- v0.61
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
---

# opa v0.61 Release Notes

Source: [v0.61.0](https://github.com/open-policy-agent/opa/releases/tag/v0.61.0)

This release contains a mix of new features and bugfixes.

### Runtime, SDK

- Adding `--v1-compatible` flag to all previously unsupported command line commands ([#6520](https://github.com/open-policy-agent/opa/issues/6520)) authored by @johanfylling
- Don't load files in tarball exceeding `size_limit_bytes` ([#6514](https://github.com/open-policy-agent/opa/issues/6514)) authored by @anderseknert reported by @dolevf
- Allow TLS cipher suites to be set for the OPA server ([#6537](https://github.com/open-policy-agent/opa/pull/6537)) authored by @ashutosh-narkar
- Removing deprecated fields and functions related to rego-v1 compatibility ([#6542](https://github.com/open-policy-agent/opa/pull/6542)) authored by @johanfylling
- bundle: Make func newDescriptor and withCloser public ([#6517](https://github.com/open-policy-agent/opa/pull/6517)) authored by @antgubarev
- runtime/logging: Do not panic when rctx is missing ([#6506](https://github.com/open-policy-agent/opa/pull/6506)) authored by @srenatus

### Topdown

- topdown: Clean expired `http.send` cache entries periodically ([#5320](https://github.com/open-policy-agent/opa/issues/5320)) authored by @rudrakhp reported by @lukyer

### Docs

- docs: Add documentation for new cache config parameters ([#6518](https://github.com/open-policy-agent/opa/pull/6518)) authored by @rudrakhp
- docs: Update docker-authorization.md to use new plugin version ([#6539](https://github.com/open-policy-agent/opa/pull/6539)) authored by @denis-accesa
- docs: Fix a typo in _index.md ([#6491](https://github.com/open-policy-agent/opa/pull/6491)) authored by @trungnguyen
- docs: Add a new debugging page ([#6513](https://github.com/open-policy-agent/opa/pull/6513)) authored by @charlieegan3
- docs: Update log masking policy examples to be Rego v1 compatible ([#6545](https://github.com/open-policy-agent/opa/pull/6545)) authored by @ashutosh-narkar
- docs: Update version for non docs pages ([#6526](https://github.com/open-policy-agent/opa/pull/6526)) authored by @charlieegan3
- Integrations, Ecosystem:
  - docs: Add dependency-management-data logo ([#6543](https://github.com/open-policy-agent/opa/pull/6543)) authored by @jamietanna
  - docs: Updated Rond links ([#6524](https://github.com/open-policy-agent/opa/pull/6524)) authored by @ugho16
  - docs: Correctly size integration logos ([#6544](https://github.com/open-policy-agent/opa/pull/6544)) authored by @charlieegan3
  - docs: Validate ecosystem keys ([#6522](https://github.com/open-policy-agent/opa/pull/6522)) authored by @charlieegan3

### Miscellaneous

- linters+testdata: Reformat all yaml testcases for linting. ([#6511](https://github.com/open-policy-agent/opa/pull/6511)) authored by @philipaconrad
- Dependency updates, notably:
  - bump github.com/containerd/containerd from 1.7.11 to 1.7.12
  - bump github.com/go-logr/logr from 1.3.0 to 1.4.1
  - bump github.com/google/uuid from 1.5.0 to 1.6.0
  - bump github.com/prometheus/client_golang from v1.16.0 to v1.18.0
  - bump google.golang.org/grpc from 1.60.1 to 1.61.0

