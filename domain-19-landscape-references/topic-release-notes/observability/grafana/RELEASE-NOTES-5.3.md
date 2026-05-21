---
title: grafana v5.3 Release Notes
description: grafana v5.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- mysql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v5.3 Release Notes 是什么
- 如何 grafana v5.3 Release Notes
trigger_keywords:
- grafana
- v5.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
- mysql-basics
---

# grafana v5.3 Release Notes

Source: [v5.3.4](https://github.com/grafana/grafana/releases/tag/v5.3.4)

[Download Page](https://grafana.com/grafana/download/5.3.4)
[Installation Guide](http://docs.grafana.org/installation/)

[Release Notes](https://community.grafana.com/t/release-notes-v5-3-x/10244)

# 5.3.4 (2018-11-13)

* **Alerting**: Delete alerts when parent folder was deleted [#13322](https://github.com/grafana/grafana/issues/13322)
* **MySQL**: Fix `$__timeFilter()` should respect local time zone [#13769](https://github.com/grafana/grafana/issues/13769)
* **Dashboard**: Fix datasource selection in panel by enter key [#13932](https://github.com/grafana/grafana/issues/13932)
* **Graph**: Fix table legend height when positioned below graph and using Internet Explorer 11 [#13903](https://github.com/grafana/grafana/issues/13903)
* **Dataproxy**: Drop origin and referer http headers [#13328](https://github.com/grafana/grafana/issues/13328) [#13949](https://github.com/grafana/grafana/issues/13949), thx [@roidelapluie](https://github.com/roidelapluie)