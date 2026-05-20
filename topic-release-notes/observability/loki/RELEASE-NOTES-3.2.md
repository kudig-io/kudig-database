---
title: loki v3.2 Release Notes
description: loki v3.2 Release Notes — Kubernetes 生产运维知识库
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
- loki v3.2 Release Notes 是什么
- 如何 loki v3.2 Release Notes
trigger_keywords:
- loki
- v3.2
- Release
- Notes
- release
- notes
---

# loki v3.2 Release Notes

Source: [v3.2.2](https://github.com/grafana/loki/releases/tag/v3.2.2)

## [3.2.2](https://github.com/grafana/loki/compare/v3.2.1...v3.2.2) (2024-12-04)


### ⚠ BREAKING CHANGES

- **promtail:** Remove `wget` from Promtail docker image (backport release-3.2.x) ([#15145](https://github.com/grafana/loki/issues/15145))

### Bug Fixes

-  **logql:** Updated JSONExpressionParser not to unescape extracted values if it is JSON object.  ([#14499](https://github.com/grafana/loki/issues/14499)).
- **storage:** Have GetObject check for canceled context. S3ObjectClient.GetObject incorrectly returned nil, 0, nil when the provided context is already canceled ([#14420](https://github.com/grafana/loki/issues/14420)).

### Miscellaneous Chores

- **promtail:** Switch Promtail base image from Debian to Ubuntu to fix critical security issues ([#15195](https://github.com/grafana/loki/issues/15195)).
- **docker:** Move from base-nossl to static. This PR removes the inclusion of glibc into most of the Docker images created by the Loki build system. ([#15203](https://github.com/grafana/loki/issues/15203)).

