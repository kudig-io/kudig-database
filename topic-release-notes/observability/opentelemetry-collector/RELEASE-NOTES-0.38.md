# opentelemetry-collector v0.38 Release Notes

Source: [v0.38.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.38.0)

## 🛑 Breaking changes 🛑

- Removed `configauth.HTTPClientAuthenticator` and `configauth.GRPCClientAuthenticator` in favor of `configauth.ClientAuthenticator`. (#4255)
- Rename `parserprovider.MapProvider` as `config.MapProvider`. (#4178)
- Rename `parserprovider.Watchable` as `config.WatchableMapProvider`. (#4178)
- Remove deprecated no-op flags to setup Collector's logging "--log-level", "--log-profile", "--log-format". (#4213)
- Move `cmd/pdatagen` as internal package `model/internal/cmd/pdatagen`. (#4243)
- Use directly the ComponentID in configauth. (#4238)
- Refactor configauth, getters use the map instead of iteration. (#4234)
- Change scraperhelper to follow the recommended append model for pdata. (#4202)

## 💡 Enhancements 💡

- Update proto to 0.11.0. (#4209)
- Change pdata to use the newly added [Traces|Metrics|Logs]Data. (#4214)
- Add ExponentialHistogram field to pdata. (#4219)
- Make sure otlphttp exporter tests include TraceID and SpanID. (#4268)
- Use multimod tool in release process. (#4229)
- Change queue metrics to use opencensus metrics instead of stats, close to otel-go. (#4220)
- Make receiver data delivery guarantees explicit (#4262)
- Simplify unmarshal logic by adding more supported hooks. (#4237)
- Add unmarshaler for otlpgrpc.[*]Request and otlpgrp.[*]Response (#4215)