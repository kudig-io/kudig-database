# opentelemetry-collector v0.88 Release Notes

Source: [v0.88.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.88.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.88.0

## User facing changes

### 💡 Enhancements 💡

- `fanoutconsumer`: Enable runtime assertions to catch incorrect pdata mutations in the components claiming as non-mutating pdata. (#6794)
  This change enables the runtime assertions to catch unintentional pdata mutations in components that are claimed
  as non-mutating pdata. Without these assertions, runtime errors may still occur, but thrown by unrelated components, 
  making it very difficult to troubleshoot.
  

### 🧰 Bug fixes 🧰

- `exporterhelper`: make enqueue failures available for otel metrics (#8673)
- `exporterhelper`: Fix nil pointer dereference when stopping persistent queue after a start encountered an error (#8718)
- `cmd/builder`: Fix ocb ignoring `otelcol_version` when set to v0.86.0 or later (#8692)

## API changes

### 💡 Enhancements 💡

- `pdata`: Add IsReadOnly() method to p[metrics|logs|traces].[Metrics|Logs|Spans] pdata structs allowing to check if the struct is read-only. (#6794)