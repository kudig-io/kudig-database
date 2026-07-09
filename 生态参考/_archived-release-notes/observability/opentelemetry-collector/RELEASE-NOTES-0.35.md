---
title: opentelemetry-collector v0.35 Release Notes
description: opentelemetry-collector v0.35 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.35 Release Notes 是什么
- 如何 opentelemetry-collector v0.35 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.35 Release Notes

Source: [v0.35.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.35.0)

## v0.35.0 Beta

## 🛑 Breaking changes 🛑

- Remove the legacy [[gRPC|gRPC]] port(`55680`) support in OTLP receiver (#3966)
- Rename configparser.Parser to configparser.ConfigMap (#3964)
- Remove obsreport.ScraperContext, embed into StartMetricsOp (#3969)
- Remove dependency on deprecated go.[[OpenTelemetry|opentelemetry]].io/otel/oteltest (#3979)
- Remove deprecated pdata.AttributeValueToString (#3953)
- Remove deprecated pdata.TimestampFromTime. Closes: #3925 (#3935)

## 💡 Enhancements 💡

- Add TelemetryCreateSettings (#3984)
- Only initialize collector telemetry once (#3918)
- Add trace context info to LogRecord log (#3959)
- Add new view for AWS ECS health check extension. (#3776)

<!-- risk-assessed -->
