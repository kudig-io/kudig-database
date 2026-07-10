---
title: opentelemetry-collector v0.95 Release Notes
description: opentelemetry-collector v0.95 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.95 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.95 Release Notes 是什么
- 如何 opentelemetry-collector v0.95 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.95
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.95 Release Notes

Source: [v0.95.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.95.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.95.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `all`: scope name for all generated Meter/Tracer funcs now includes full package name (#9494)

### 💡 Enhancements 💡

- `confighttp`: Adds support for Snappy decompression of HTTP requests. (#7632)
- `configretry`: Validate `max_elapsed_time`, ensure it is larger than `max_interval` and `initial_interval` respectively. (#9489)
- `configopaque`: Mark module as stable (#9167)
- `otlphttpexporter`: Add support for json content encoding when exporting telemetry (#6945)
- `confmap/converter/expandconverter, confmap/provider/envprovider, confmap/provider/fileprovider, confmap/provider/httprovider, confmap/provider/httpsprovider, confmap/provider/yamlprovider`: Split confmap.Converter and confmap.Provider implementation packages out of confmap. (#4759, #9460)

## API Changelog

### 🛑 Breaking changes 🛑

- `all`: Bump minimum go version to go 1.21 (#9507)
- `[[Service|service]]/telemetry`: Delete generated_config types, use go.opentelemetry.io/contrib/config types instead (#9546)
- `configcompression`: Remove deprecated `configcompression` types, constants and methods. (#9388)
- `component`: Remove `host.ReportFatalError` (#6344)
- `configgrpc`: Remove deprecated `configgrpc.ServerConfig.ToListener` (#9481)
- `confmap`: Remove deprecated `confmap.WithErrorUnused` (#9484)

### 🚩 Deprecations 🚩

- `confignet`: Deprecate `confignet.NetAddr` and `confignet.TCPAddr` in favor of `confignet.AddrConfig` and `confignet.TCPAddrConfig`. (#9509)
- `config/configgrpc`: Deprecate `configgrpc.ClientConfig.SanitizedEndpoint`, `configgrpc.ServerConfig.ToListener` and `configgrpc.ServerConfig.ToListenerContext` (#9481, #9482)
- `scraperhelper`: Deprecate ScraperControllerSettings, use ControllerConfig instead (#6767)


<!-- risk-assessed -->
