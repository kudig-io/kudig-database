# opentelemetry-collector v0.23 Release Notes

Source: [v0.23.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.23.0)

# v0.23.0 Beta

## 🛑 Breaking changes 🛑

- Move fanout consumers to fanoutconsumer package (#2615)
- Rename ExporterObsReport to Exporter (#2658)
- Rename ProcessorObsReport to Processor (#2657)
- Remove ValidateConfig and add Validate on the Config struct (#2665)
- Rename pdata Size to OtlpProtoSize (#2726)
- Rename [Traces|Metrics|Logs]Consumer to [Traces|Metrics|Logs] (#2761)
- Remove public access for `componenttest.Example*` components:
  - Users of these structs for testing configs should use the newly added `componenttest.Nop*` (update all components name in the config `example*` -> `nop` and use `componenttest.NopComponents()`).
  - Users of these structs for sink like behavior should use `consumertest.*Sink`.

## 💡 Enhancements 💡

- `hostmetrics` receiver: List labels along with respective metrics in metadata (#2662)
- `exporter` helper: Remove obsreport.ExporterContext, always add exporter name as a tag to the metrics (#2682)
- `jaeger` exporter: Change to not use internal data (#2698)
- `kafka` receiver: Change to not use internal data (#2697)
- `zipkin` receiver: Change to not use internal data (#2699)
- `kafka` exporter: Change to not use internal data (#2696)
- Ensure that extensions can be created and started multiple times (#2679)
- Use otlp request in logs wrapper, hide members in the wrapper (#2692)
- Add MetricsWrapper to dissallow access to internal representation (#2693)
- Add TracesWrapper to dissallow access to internal representation (#2721)
- Allow multiple OTLP receivers to be created (#2743)

## 🧰 Bug fixes 🧰

- `prometheus` exporter: Fix to work with standard labels that follow the naming convention of using periods instead of underscores (#2707)
- Propagate name and transport for `prometheus` receiver and exporter (#2680)
- `zipkin` receiver: Ensure shutdown correctness (#2765)


## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.
