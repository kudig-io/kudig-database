# opentelemetry-collector v0.75 Release Notes

Source: [v0.75.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.75.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.75.0

### 🛑 Breaking changes 🛑

- `featuregate`: Remove deprecated featuregate.FlagValue (#7401)

### 💡 Enhancements 💡

- `provider`: Added userfriendly error on incorrect type. (#7399)

### 🧰 Bug fixes 🧰

- `loggingexporter`: Fix display of bucket boundaries of exponential histograms to correctly reflect inclusive/exclusive bounds. (#7445)
- `exporterhelper`: Fix a deadlock in persistent queue initialization (#7400)