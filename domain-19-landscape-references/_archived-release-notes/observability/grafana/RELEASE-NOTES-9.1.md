---
title: grafana v9.1 Release Notes
description: grafana v9.1 Release Notes — Kubernetes 生产运维知识库
summary: grafana v9.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- grafana
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v9.1 Release Notes 是什么
- 如何 grafana v9.1 Release Notes
trigger_keywords:
- grafana
- v9.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
---



# grafana v9.1 Release Notes

Source: [v9.1.8](https://github.com/grafana/grafana/releases/tag/v9.1.8)


[Download page](https://grafana.com/grafana/download/9.1.8)
[What's new highlights](https://grafana.com/docs/grafana/latest/whatsnew/)


### Features and enhancements

- **Alerting:** Update imported [[Prometheus|prometheus]] alertmanager version. Backport (#56228). [#56429](https://github.com/grafana/grafana/pull/56429), [@joeblubaugh](https://github.com/joeblubaugh)
- **Chore:** Upgrade Go to 1.19.2. [#56355](https://github.com/grafana/grafana/pull/56355), [@sakjur](https://github.com/sakjur)

### Bug fixes

- **Alerting:** Fix evaluation interval validation. [#56115](https://github.com/grafana/grafana/pull/56115), [@konrad147](https://github.com/konrad147)
- **Alerting:** Fix migration to create rules with group index 1. [#56511](https://github.com/grafana/grafana/pull/56511), [@yuri-tceretian](https://github.com/yuri-tceretian)
- **Alerting:** Fix migration to not add label "alertname". [#56509](https://github.com/grafana/grafana/pull/56509), [@yuri-tceretian](https://github.com/yuri-tceretian)
- **Azure Monitor:** Fix empty Logs response for Alerting. [#56378](https://github.com/grafana/grafana/pull/56378), [@andresmgot](https://github.com/andresmgot)
- **Azure Monitor:** Fix subscription selector when changing data sources. [#56284](https://github.com/grafana/grafana/pull/56284), [@andresmgot](https://github.com/andresmgot)
- **Caching:** Fix wrong memcached setting name in defaults. (Enterprise)
- **Google Cloud Monitoring:** Fix bucket bound for distributions. [#56565](https://github.com/grafana/grafana/pull/56565), [@andresmgot](https://github.com/andresmgot)

