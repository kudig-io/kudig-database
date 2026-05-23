---
title: prometheus v2.21 Release Notes
description: prometheus v2.21 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.21 Release Notes 是什么
- 如何 prometheus v2.21 Release Notes
trigger_keywords:
- prometheus
- v2.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
created: "2026-05-23"
---

# [[Prometheus|prometheus]] v2.21 Release Notes

Source: [v2.21.0](https://github.com/prometheus/prometheus/releases/tag/v2.21.0)

This release is built with Go 1.15, which deprecates [X.509 CommonName](https://golang.org/doc/go1.15#commonname)
in TLS certificates validation.

In the unlikely case that you use the [[gRPC|gRPC]] API v2 (which is limited to TSDB
admin commands), please note that we will remove this experimental API in the
next minor release 2.22.

* [CHANGE] Disable HTTP/2 because of concerns with the Go HTTP/2 client. #7588 #7701
* [CHANGE] PromQL: `query_log_file` path is now relative to the config file. #7701
* [CHANGE] Promtool: Replace the tsdb command line tool by a promtool tsdb subcommand. #6088
* [CHANGE] Rules: Label `rule_group_iterations` metric with group name. #7823
* [FEATURE] Eureka SD: New service discovery. #3369
* [FEATURE] Hetzner SD: New service discovery. #7822
* [FEATURE] Kubernetes SD: Support Kubernetes EndpointSlices. #6838
* [FEATURE] Scrape: Add per scrape-config targets limit. #7554
* [ENHANCEMENT] Support composite durations in PromQL, config and UI, e.g. 1h30m. #7713 #7833
* [ENHANCEMENT] DNS SD: Add SRV record target and port meta labels. #7678
* [ENHANCEMENT] Docker Swarm SD: Support tasks and service without published ports. #7686
* [ENHANCEMENT] PromQL: Reduce the amount of data queried by remote read when a subquery has an offset. #7667
* [ENHANCEMENT] Promtool: Add `--time` option to query instant command. #7829
* [ENHANCEMENT] UI: Respect the `--web.page-title` parameter in the React UI. #7607
* [ENHANCEMENT] UI: Add duration, labels, annotations to alerts page in the React UI. #7605
* [ENHANCEMENT] UI: Add duration on the React UI rules page, hide annotation and labels if empty. #7606
* [BUGFIX] API: Deduplicate series in /api/v1/series. #7862
* [BUGFIX] PromQL: Drop metric name in bool comparison between two instant vectors. #7819
* [BUGFIX] PromQL: Exit with an error when time parameters can't be parsed. #7505
* [BUGFIX] Remote read: Re-add accidentally removed tracing for remote-read requests. #7916
* [BUGFIX] Rules: Detect extra fields in rule files. #7767
* [BUGFIX] Rules: Disallow overwriting the metric name in the `labels` section of recording rules. #7787
* [BUGFIX] Rules: Keep evaluation timestamp across reloads. #7775
* [BUGFIX] Scrape: Do not stop scrapes in progress during reload. #7752
* [BUGFIX] TSDB: Fix `chunks.HeadReadWriter: maxt of the files are not set` error. #7856
* [BUGFIX] TSDB: Delete blocks atomically to prevent corruption when there is a panic/crash during deletion. #7772
* [BUGFIX] Triton SD: Fix a panic when triton_sd_config is nil. #7671
* [BUGFIX] UI: Fix react UI bug with series going on and off. #7804
* [BUGFIX] UI: Fix styling bug for target labels with special names in React UI. #7902
* [BUGFIX] Web: Stop CMUX and GRPC servers even with stale connections, preventing the server to stop on SIGTERM. #7810
