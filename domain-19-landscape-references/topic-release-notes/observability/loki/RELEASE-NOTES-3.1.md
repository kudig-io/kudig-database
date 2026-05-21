---
title: loki v3.1 Release Notes
description: loki v3.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- loki v3.1 Release Notes 是什么
- 如何 loki v3.1 Release Notes
trigger_keywords:
- loki
- v3.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
- logging-basics
---

# loki v3.1 Release Notes

Source: [v3.1.2](https://github.com/grafana/loki/releases/tag/v3.1.2)

## [3.1.2](https://github.com/grafana/loki/compare/v3.1.1...v3.1.2) (2024-10-17)


### Bug Fixes

* **config:** Copy Alibaba and IBM object storage configuration from common ([#14316](https://github.com/grafana/loki/issues/14316)) ([7184d45](https://github.com/grafana/loki/commit/7184d45d8e080874feea8bfd223dedf5f20d3836))
* **logql:** updated JSONExpressionParser not to unescape extracted values if it is JSON object. (backport release-3.1.x) ([#14503](https://github.com/grafana/loki/issues/14503)) ([759f9c8](https://github.com/grafana/loki/commit/759f9c8525227bb1272771a40429d12e015874d9))
* **loki/production/docker-compose:** upgrade loki and grafana production image tags to 3.1.1 ([#14025](https://github.com/grafana/loki/issues/14025)) ([36fe29e](https://github.com/grafana/loki/commit/36fe29eb334d8300265ca437c0acb423a01c5041))
* Revert build image to Debian Bullseye to fix libc version issue in Promtail ([#14387](https://github.com/grafana/loki/issues/14387)) ([05b6a65](https://github.com/grafana/loki/commit/05b6a65f8bf00b880f17465553b1adaf0cf56d60))
* **storage/chunk/client/aws:** have GetObject check for canceled context (backport release-3.1.x) ([#14421](https://github.com/grafana/loki/issues/14421)) ([f3d69ff](https://github.com/grafana/loki/commit/f3d69ffa960c91c0239436a32bb0aa578c0f022a))