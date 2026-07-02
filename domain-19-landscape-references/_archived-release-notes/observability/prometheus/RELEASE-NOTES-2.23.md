---
title: prometheus v2.23 Release Notes
description: prometheus v2.23 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- opa
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
- prometheus v2.23 Release Notes 是什么
- 如何 prometheus v2.23 Release Notes
trigger_keywords:
- prometheus
- v2.23
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Prometheus|prometheus]] v2.23 Release Notes

Source: [v2.23.0](https://github.com/prometheus/prometheus/releases/tag/v2.23.0)

* [CHANGE] UI: Make the React UI default. #8142
* [CHANGE] Remote write: The following metrics were removed/renamed in remote write. #6815
     - `prometheus_remote_storage_succeeded_samples_total` was removed and `prometheus_remote_storage_samples_total` was introduced for all the samples attempted to send.
     - `prometheus_remote_storage_sent_bytes_total` was removed and replaced with `prometheus_remote_storage_samples_bytes_total` and `prometheus_remote_storage_metadata_bytes_total`.
     - `prometheus_remote_storage_failed_samples_total` -> `prometheus_remote_storage_samples_failed_total` .
     - `prometheus_remote_storage_retried_samples_total` -> `prometheus_remote_storage_samples_retried_total`.
     - `prometheus_remote_storage_dropped_samples_total` -> `prometheus_remote_storage_samples_dropped_total`.
     - `prometheus_remote_storage_pending_samples` -> `prometheus_remote_storage_samples_pending`.
* [CHANGE] Remote: Do not collect non-initialized timestamp metrics. #8060
* [FEATURE] [EXPERIMENTAL] Remote write: Allow metric metadata to be propagated via remote write. The following new metrics were introduced: `prometheus_remote_storage_metadata_total`, `prometheus_remote_storage_metadata_failed_total`, `prometheus_remote_storage_metadata_retried_total`, `prometheus_remote_storage_metadata_bytes_total`. #6815
* [ENHANCEMENT] Remote write: Added a metric `prometheus_remote_storage_max_samples_per_send` for remote write. #8102
* [ENHANCEMENT] TSDB: Make the snapshot directory name always the same length. #8138
* [ENHANCEMENT] TSDB: Create a checkpoint only once at the end of all head compactions. #8067
* [ENHANCEMENT] TSDB: Avoid Series API from hitting the chunks. #8050 
* [ENHANCEMENT] TSDB: Cache label name and last value when adding series during compactions making compactions faster. #8192
* [ENHANCEMENT] PromQL: Improved performance of Hash method making queries a bit faster. #8025
* [ENHANCEMENT] promtool: `tsdb list` now prints block sizes. #7993
* [ENHANCEMENT] promtool: Calculate mint and maxt per test avoiding unnecessary calculations. #8096
* [ENHANCEMENT] SD: Add filtering of services to Docker Swarm SD. #8074
* [BUGFIX] React UI: Fix button display when there are no panels. #8155
* [BUGFIX] PromQL: Fix timestamp() method for vector selector inside parenthesis. #8164
* [BUGFIX] PromQL: Don't include rendered expression on PromQL parse errors. #8177
* [BUGFIX] web: Fix panic with double close() of channel on calling `/-/quit/`. #8166
* [BUGFIX] TSDB: Fixed WAL corruption on partial writes within a page causing `invalid checksum` error on WAL replay. #8125
* [BUGFIX] Update config metrics `prometheus_config_last_reload_successful` and `prometheus_config_last_reload_success_timestamp_seconds` right after initial validation before starting TSDB.
* [BUGFIX] promtool: Correctly detect duplicate label names in exposition.


<!-- risk-assessed -->
