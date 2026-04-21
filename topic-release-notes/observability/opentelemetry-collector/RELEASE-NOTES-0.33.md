# opentelemetry-collector v0.33 Release Notes

Source: [v0.33.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.33.0)

## v0.33.0 Beta

## 🛑 Breaking changes 🛑

- Rename `configloader` interface to `configunmarshaler` (#3774)
- Remove `LabelsMap` from all the metrics points (#3706)
- Update generated K8S attribute labels to fix capitalization (#3823) 

## 💡 Enhancements 💡

- Collector has now full support for metrics proto v0.9.0.
