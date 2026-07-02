---
title: prometheus v2.42 Release Notes
description: prometheus v2.42 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.42 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.42 Release Notes 是什么
- 如何 prometheus v2.42 Release Notes
trigger_keywords:
- prometheus
- v2.42
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Prometheus|prometheus]] v2.42 Release Notes

Source: [v2.42.0](https://github.com/prometheus/prometheus/releases/tag/v2.42.0)

This release comes with a bunch of feature coverage for native histograms and breaking changes.

If you are trying native histograms already, we recommend you remove the `wal` directory when upgrading.
Because the old WAL record for native histograms is not backward compatible in v2.42.0, this will lead to some data loss for the latest data.

Additionally, if you scrape "float histograms" or use recording rules on native histograms in v2.42.0 (which writes float histograms),
it is a one-way street since older versions do not support float histograms.

* [CHANGE] **breaking** TSDB: Changed WAL record format for the experimental native histograms. #11783
* [FEATURE] Add 'keep_firing_for' field to alerting rules. #11827
* [FEATURE] Promtool: Add support of selecting timeseries for TSDB dump. #11872
* [ENHANCEMENT] Agent: Native histogram support. #11842
* [ENHANCEMENT] Rules: Support native histograms in recording rules. #11838
* [ENHANCEMENT] SD: Add container ID as a meta label for pod targets for [[Kubernetes|Kubernetes]]. #11844
* [ENHANCEMENT] SD: Add VM size label to azure [[Service|service]] discovery. #11650
* [ENHANCEMENT] Support native histograms in federation. #11830
* [ENHANCEMENT] TSDB: Add gauge histogram support. #11783 #11840 #11814
* [ENHANCEMENT] TSDB/Scrape: Support FloatHistogram that represents buckets as float64 values. #11522 #11817 #11716
* [ENHANCEMENT] UI: Show individual scrape pools on /targets page. #11142


<!-- risk-assessed -->
