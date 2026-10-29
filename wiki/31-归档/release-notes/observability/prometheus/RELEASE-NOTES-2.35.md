---
title: prometheus v2.35 Release Notes
description: prometheus v2.35 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
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
- prometheus v2.35 Release Notes 是什么
- 如何 prometheus v2.35 Release Notes
trigger_keywords:
- prometheus
- v2.35
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




# [[Prometheus|prometheus]] v2.35 Release Notes

Source: [v2.35.0](https://github.com/prometheus/prometheus/releases/tag/v2.35.0)

This Prometheus release is built with go1.18, which contains two noticeable changes related to TLS:

1. [TLS 1.0 and 1.1 disabled by default client-side](https://go.dev/doc/go1.18#tls10). 
Prometheus users can override this with the `min_version` parameter of [tls_config](https://prometheus.io/docs/prometheus/latest/configuration/configuration/#tls_config).
1. Certificates signed with the SHA-1 hash function are rejected](https://go.dev/doc/go1.18#sha1). This doesn't apply to self-signed root certificates.

----

* [CHANGE] TSDB: Delete `*.tmp` WAL files when Prometheus starts. #10317
* [CHANGE] promtool: Add new flag `--lint` (enabled by default) for the commands `check rules` and `check config`, resulting in a new exit code (`3`) for linter errors. #10435
* [FEATURE] Support for automatically setting the variable `GOMAXPROCS` to the container CPU limit. Enable with the flag `--enable-feature=auto-gomaxprocs`. #10498
* [FEATURE] PromQL: Extend statistics with total and peak number of samples in a query. Additionally, per-step statistics are available with  --enable-feature=promql-per-step-stats and using `stats=all` in the query API. 
Enable with the flag `--enable-feature=per-step-stats`. #10369
* [ENHANCEMENT] Prometheus is built with Go 1.18. #10501
* [ENHANCEMENT] TSDB: more efficient sorting of postings read from WAL at startup. #10500
* [ENHANCEMENT] Azure SD: Add metric to track Azure SD failures. #10476
* [ENHANCEMENT] Azure SD: Add an optional `resource_group` configuration. #10365
* [ENHANCEMENT] Kubernetes SD: Support `discovery.k8s.io/v1` `EndpointSlice` (previously only `discovery.k8s.io/v1beta1` `EndpointSlice` was supported). #9570
* [ENHANCEMENT] Kubernetes SD: Allow attaching node metadata to discovered pods. #10080
* [ENHANCEMENT] OAuth2: Support for using a proxy URL to fetch OAuth2 tokens. #10492
* [ENHANCEMENT] Configuration: Add the ability to disable HTTP2. #10492
* [ENHANCEMENT] Config: Support overriding minimum TLS version. #10610
* [BUGFIX] Kubernetes SD: Explicitly include gcp auth from k8s.io. #10516
* [BUGFIX] Fix OpenMetrics parser to sort uppercase labels correctly. #10510
* [BUGFIX] UI: Fix scrape interval and duration tooltip not showing on target page. #10545
* [BUGFIX] Tracing/GRPC: Set TLS credentials only when insecure is false. #10592
* [BUGFIX] Agent: Fix ID collision when loading a WAL with multiple segments. #10587
* [BUGFIX] Remote-write: Fix a deadlock between Batch and flushing the queue. #10608


<!-- risk-assessed -->
