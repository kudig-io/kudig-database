# opentelemetry-collector v0.74 Release Notes

Source: [v0.74.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.74.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.74.0

### 🛑 Breaking changes 🛑

- `consumererror`: Remove deprecated funcs in consumererror (#7357)

### 🚩 Deprecations 🚩

- `featuregate`: Deprecate `FlagValue` in favor of `NewFlag`. (#7042)

### 💡 Enhancements 💡

- `service`: Enable connectors by default by moving service.connectors featuregate to beta (#7369)