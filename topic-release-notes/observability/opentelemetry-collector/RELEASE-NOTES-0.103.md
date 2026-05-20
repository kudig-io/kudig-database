---
title: opentelemetry-collector v0.103 Release Notes
description: opentelemetry-collector v0.103 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.103 Release Notes 是什么
- 如何 opentelemetry-collector v0.103 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.103
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.103 Release Notes

Source: [v0.103.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.103.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.103.0

## End User Changelog


### 🛑 Breaking changes 🛑

- `exporter/debug`: Disable sampling by default (#9921)
  To restore the behavior that was previously the default, set `sampling_thereafter` to `500`.

### 💡 Enhancements 💡

- `cmd/builder`: Allow setting `otelcol.CollectorSettings.ResolverSettings.DefaultScheme` via the builder's `conf_resolver.default_uri_scheme` configuration option (#10296)
- `mdatagen`: add support for optional internal metrics (#10316)
- `otelcol/expandconverter`: Add `confmap.unifyEnvVarExpansion` feature gate to allow enabling Collector/Configuration SIG environment variable expansion rules. (#10391)
  When enabled, this feature gate will:
  - Disable expansion of BASH-style env vars (`$FOO`)
  - `${FOO}` will be expanded as if it was `${env:FOO}
  See https://github.com/open-telemetry/opentelemetry-collector/blob/main/docs/rfcs/env-vars.md for more details.
  
- `confmap`: Add `confmap.unifyEnvVarExpansion` feature gate to allow enabling Collector/Configuration SIG environment variable expansion rules. (#10259)
  When enabled, this feature gate will:
    - Disable expansion of BASH-style env vars (`$FOO`)
    - `${FOO}` will be expanded as if it was `${env:FOO}
  See https://github.com/open-telemetry/opentelemetry-collector/blob/main/docs/rfcs/env-vars.md for more details.
  
- `confighttp`: Allow the compression list to be overridden (#10295)
  Allows Collector administrators to control which compression algorithms to enable for HTTP-based receivers.
- `configgrpc`: Revert the zstd compression for gRPC to the third-party library we were using previously. (#10394)
  We switched back to our compression logic for zstd when a CVE was found on the third-party library we were using. Now that the third-party library has been fixed, we can revert to that one. For end-users, this has no practical effect. The reproducers for the CVE were tested against this patch, confirming we are not reintroducing the bugs.
- `confmap`: Adds alpha `confmap.strictlyTypedInput` feature gate that enables strict type checks during configuration resolution (#9532)
  When enabled, the configuration resolution system will:
  - Stop doing most kinds of implicit type casting when resolving configuration values
  - Use the original string representation of configuration values if the ${} syntax is used in inline position
  
- `confighttp`: Use `confighttp.ServerConfig` as part of zpagesextension. See [https://github.com/open-telemetry/opentelemetry-collector/blob/main/config/confighttp/README.md#server-configuration](server configuration) options. (#9368)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Fix potential deadlock in the batch sender (#10315)
- `expandconverter`: Fix bug where an warning was logged incorrectly. (#10392)
- `exporterhelper`: Fix a bug when the retry and timeout logic was not applied with enabled batching. (#10166)
- `exporterhelper`: Fix a bug where an unstarted batch_sender exporter hangs on shutdown (#10306)
- `exporterhelper`: Fix small batch due to unfavorable goroutine scheduling in batch sender (#9952)
- `confmap`: Fix issue where structs with only yaml tags were not marshaled correctly. (#10282)

## Go API Changelog

### 🛑 Breaking changes 🛑

- `component`: Remove deprecated `component.UnmarshalConfig` (#7102)
- `confighttp`: Use `confighttp.ServerConfig` as part of zpagesextension.Config. Previously the extension used `confignet.TCPAddrConfig` (#9368)

### 🚩 Deprecations 🚩

- `connector`: Deprecate CreateSettings and NewNopCreateSettings (#9428)
  The following methods are being renamed:
  - connector.CreateSettings -> connector.Settings
  - connector.NewNopCreateSettings -> connector.NewNopSettings
  
- `exporter`: Deprecate CreateSettings and NewNopCreateSettings (#9428)
  The following methods are being renamed:
  - exporter.CreateSettings -> exporter.Settings
  - exporter.NewNopCreateSettings -> exporter.NewNopSettings
  
- `extension`: Deprecate CreateSettings and NewNopCreateSettings (#9428)
  The following methods are being renamed:
  - extension.CreateSettings -> extension.Settings
  - extension.NewNopCreateSettings -> extension.NewNopSettings
  
- `processor`: Deprecate CreateSettings and NewNopCreateSettings (#9428)
  The following methods are being renamed:
  - processor.CreateSettings -> processor.Settings
  - processor.NewNopCreateSettings -> processor.NewNopSettings
  
- `receiver`: Deprecate CreateSettings and NewNopCreateSettings (#9428)
  The following methods are being renamed:
  - receiver.CreateSettings -> receiver.Settings
  - receiver.NewNopCreateSettings -> receiver.NewNopSettings
  
- `configauth`: Deprecate `GetClientAuthenticator` and `GetServerAuthenticator`, use `GetClientAuthenticatorContext` and `GetServerAuthenticatorContext` instead. (#9808)
- `confighttp`: Deprecate `ClientConfig.CustomRoundTripper` (#8627)
  Set the `Transport` field on the `*http.Client` object returned from `(ClientConfig).ToClient` instead.
- `filter`: Deprecate the `filter.CombinedFilter` struct (#10348)
- `otelcol`: Deprecate `otelcol.NewCommand`. Use `otelcol.NewCommandMustProviderSettings` instead. (#10359)
- `otelcoltest`: Deprecate `LoadConfig` and `LoadConfigAndValidate`. Use `LoadConfigWithSettings` and `LoadConfigAndValidateWithSettings` instead (#10359)

### 💡 Enhancements 💡

- `confmap`: Adds `confmap.Retrieved.AsString` method that returns the configuration value as a string (#9532)
- `confmap`: Adds `confmap.NewRetrievedFromYAML` helper to create `confmap.Retrieved` values from YAML bytes (#9532)
