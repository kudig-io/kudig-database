---
title: loki v3.3 Release Notes
description: loki v3.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- loki v3.3 Release Notes 是什么
- 如何 loki v3.3 Release Notes
trigger_keywords:
- loki
- v3.3
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

# loki v3.3 Release Notes

Source: [v3.3.4](https://github.com/grafana/loki/releases/tag/v3.3.4)

## [3.3.4](https://github.com/grafana/loki/compare/v3.3.3...v3.3.4) (2025-04-03)


### Bug Fixes

* **deps:** Move to Go 1.23.7 ([#16681](https://github.com/grafana/loki/issues/16681)) ([f6c6474](https://github.com/grafana/loki/commit/f6c6474ca6b09eaadd0c9d0f42e337b62cba3f6a))
* **deps:** update jwt and oauth2 dependencies for 3.3.x ([#17021](https://github.com/grafana/loki/issues/17021)) ([241f5aa](https://github.com/grafana/loki/commit/241f5aa6490b09b299c1136e70e5b67f85b8632d))
* **deps:** update module golang.org/x/crypto to v0.35.0 [security] (release-3.3.x) ([#16590](https://github.com/grafana/loki/issues/16590)) ([454cad2](https://github.com/grafana/loki/commit/454cad207b55e65e7b22c077eaedf84589860746))
* **deps:** update module golang.org/x/oauth2 to v0.27.0 [security] (release-3.3.x) ([#16591](https://github.com/grafana/loki/issues/16591)) ([7b35a41](https://github.com/grafana/loki/commit/7b35a41c17be1326df4324188eb43e4e88d16f8d))
* **docs:** add a note on docker configuration.md doc to explain accep… ([#16746](https://github.com/grafana/loki/issues/16746)) ([0102107](https://github.com/grafana/loki/commit/01021076ccc2bfe4812fc9116c66190334d3aa91))