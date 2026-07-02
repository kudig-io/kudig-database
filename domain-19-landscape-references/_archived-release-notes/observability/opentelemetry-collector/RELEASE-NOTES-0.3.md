---
title: opentelemetry-collector v0.3 Release Notes
description: opentelemetry-collector v0.3 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.3 Release Notes 是什么
- 如何 opentelemetry-collector v0.3 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.3 Release Notes

Source: [v0.3.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.3.0)

## Docker images:

* Core: `docker pull otel/opentelemetry-collector:0.3.0`
  * Supported open-source receivers and exporters
  * Supported processors and extensions
* [Contrib](https://github.com/open-telemetry/opentelemetry-collector-contrib): `docker pull otel/opentelemetry-collector-contrib:0.3.0`
  * Community open-source receivers and exporters including for commercial back-ends
  * Community processors and extensions

## Breaking changes

-  Make [[Prometheus|prometheus]] reciever config loading strict. #697 
Prometheus receiver will now fail fast if the config contains unused keys in it.

## Changes and fixes

- Enable best effort serve by default of Prometheus Exporter (https://github.com/orijtech/prometheus-go-metrics-exporter/pull/6)
- Fix null pointer exception in the logging exporter #743 
- Remove unnecessary condition to have at least one processor #744 

## Components

| Receivers / Exporters | Processors | Extensions |
|:---------------------:|:-----------:|:-----------:|
| [[Jaeger|Jaeger]] | Attributes | Health Check |
| OpenCensus | Batch | Performance Profiler |
| OpenTelemetry | Memory Limiter | zPages |
| Zipkin | Queued Retry | |
| | Resource | |
| | Sampling | |
| | Span | |


## Checksums

Checksums were generated with `shasum -a 256 bin/*` and are present in the checksums.txt file. 

<!-- risk-assessed -->
