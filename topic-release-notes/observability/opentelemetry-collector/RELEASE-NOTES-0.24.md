# opentelemetry-collector v0.24 Release Notes

Source: [v0.24.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.24.0)

## v0.24.0 Beta

## 🛑 Breaking changes 🛑

- Remove legacy internal metrics for memorylimiter processor, `spans_dropped` and `trace_batches_dropped` (#2841)
  - For `spans_dropped` use `processor/refused_spans` with `processor=memorylimiter`
- Rename pdata.*.[Start|End]Time to pdata.*.[Start|End]Timestamp (#2847)
- Rename pdata.DoubleExemplar to pdata.Exemplar (#2804)
- Rename pdata.DoubleHistogram to pdata.Histogram (#2797)
- Rename pdata.DoubleSummary to pdata.Summary (#2774)
- Refactor `consumererror` package (#2768)
  - Remove `PartialError` type in favor of signal-specific types
  - Rename `CombineErrors()` to `Combine()`
- Refactor `componenthelper` package (#2778)
  - Remove `ComponentSettings` and `DefaultComponentSettings()`
  - Rename `NewComponent()` to `New()`
- obsReport.NewExporter accepts a settings struct (#2668)
- Remove ErrorWaitingHost from `componenttest` (#2582)
- Move `config.Load` to use `configparser.Load` (#2796)
- Remove `configtest.NewViperFromYamlFile()`, use `config.Parser.NewParserFromFile()` (#2806)
- Move `config.ViperSubExact()` to use `config.Parser.Sub()` (#2806)
- Update LoadReceiver signature to remove unused params (#2823)
- Move `configerror.ErrDataTypeIsNotSupported` to `componenterror.ErrDataTypeIsNotSupported` (#2886)
- Rename`CreateTraceExporter` type to `CreateTracesExporter` in `exporterhelper` (#2779)
- Move `fluentbit` extension to contrib (#2795)
- Move `configmodels` to `config` (#2808)
- Move `fluentforward` receiver to contrib (#2723)

## 💡 Enhancements 💡

- `batch` processor: - Support max batch size for logs (#2736)
- Use `Endpoint` for health check extension (#2782)
- Use `confignet.TCPAddr` for `pprof` and `zpages` extensions (#2829)
- Deprecate `consumetest.New[${SIGNAL}]Nop` in favor of `consumetest.NewNop` (#2878)
- Deprecate `consumetest.New[${SIGNAL}]Err` in favor of `consumetest.NewErr` (#2878)
- Add watcher to values retrieved via config sources (#2803)
- Updates for cloud semantic conventions (#2809)
  - `cloud.infrastructure_service` -> `cloud.platform`
  - `cloud.zone` -> `cloud.availability_zone`
- Add systemd environment file for deb/rpm packages (#2822)
- Add validate interface in `configmodels` to force each component do configuration validation (#2802, #2856)
- Add `aws.ecs.task.revision` to semantic conventions list (#2816)
- Set unprivileged user to container image (#2838)
- Add New funcs for extension, exporter, processor config settings (#2872)
- Report metric about current size of the exporter retry queue (#2858)
- Allow adding new signals in `ProcessorFactory` by forcing everyone to embed `BaseProcessorFactory` (#2885)

## 🧰 Bug fixes 🧰

- `pdata.TracesFromOtlpProtoBytes`: Fixes to handle backwards compatibility changes in proto (#2798)
- `jaeger` receiver: Escape user input used in output (#2815)
- `prometheus` exporter: Ensure same time is used for updated time (#2745)
- `prometheusremotewrite` exporter: Close HTTP response body (#2875)


## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.