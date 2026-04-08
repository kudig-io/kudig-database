# opentelemetry-collector v0.45 Release Notes

Source: [v0.45.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.45.0)

## v0.45.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.45.0

### 🛑 Breaking changes 🛑

- Remove deprecated funcs in configtelemetry (#4808)
- Deprecate `service/defaultcomponents` go package (#4622)
- `otlphttp` and `otlp` exporters enable gzip compression by default (#4632)

### 💡 Enhancements 💡

- Reject invalid queue size exporterhelper (#4799)
- Transform configmapprovider.Retrieved interface to a struct (#4789)
- Added feature gate summary to zpages extension (#4834)
- Add support for reloading TLS certificates (#4737)

### 🚩 Deprecations 🚩

- Deprecate `pdata.NumberDataPoint.Type()` and `pdata.Exemplar.Type()` in favor of `NumberDataPoint.ValueType()` and
  `Exemplar.ValueType()` (#4850)