---
title: opentelemetry-collector v0.96 Release Notes
description: opentelemetry-collector v0.96 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.96 Release Notes 是什么
- 如何 opentelemetry-collector v0.96 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.96
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.96 Release Notes

Source: [v0.96.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.96.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.96.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `configgrpc`: Remove deprecated `GRPCClientSettings`, `GRPCServerSettings`, and `ServerConfig.ToListenerContext`. (#9616)
- `confighttp`: Remove deprecated `HTTPClientSettings`, `NewDefaultHTTPClientSettings`, and `CORSSettings`. (#9625)
- `confignet`: Removes deprecated `NetAddr` and `TCPAddr` (#9614)

### 💡 Enhancements 💡

- `configtls`: Add `include_system_ca_certs_pool` to configtls, allowing to load system certs and additional custom certs. (#7774)
- `otelcol`: Add `ConfigProviderSettings` to `CollectorSettings` (#4759)
  This allows passing a custom list of `confmap.Provider`s to `otelcol.NewCommand`.
- `pdata`: Update to OTLP v1.1.0 (#9587)
  Introduces Span and SpanLink flags.
- `confmap`: Update mapstructure to use a maintained fork, github.com/go-viper/mapstructure/v2. (#9634)
  See https://github.com/mitchellh/mapstructure/issues/349 for context.

### 🧰 Bug fixes 🧰

- `configretry`: Allow max_elapsed_time to be set to 0 for indefinite retries (#9641)
- `client`: Make `Metadata.Get` thread safe (#9595)

## API Changelog

### 🚩 Deprecations 🚩

- `configgrpc`: Deprecates `ToServer`.  Use `ToServerContext` instead. (#9624)
- `component`: deprecate component.ErrNilNextConsumer (#9526)
- `configtls`: Rename TLSClientSetting, TLSServerSetting, and TLSSetting based on the naming convention used in other config packages. (#9474)

### 💡 Enhancements 💡

- `receivertest`: add support for metrics in contract checker (#9551)
