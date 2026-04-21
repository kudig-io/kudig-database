# opentelemetry-collector v0.41 Release Notes

Source: [v0.41.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.41.0)

## 🛑 Breaking changes 🛑

- Remove reference to `defaultcomponents` in core and deprecate `include_core` flag (#4087)
- Remove `config.NewConfigMapFrom[File|Buffer]`, add testonly version (#4502)
- `configtls`: TLS 1.2 is the new default mininum version (#4503)
- `confighttp`: `ToServer` now accepts a `component.Host`, in line with gRPC's counterpart (#4514)
- CORS configuration for OTLP/HTTP receivers has been moved into a `cors:` block, instead of individual `cors_allowed_origins` and `cors_allowed_headers` settings (#4492)

## 💡 Enhancements 💡

- OTLP/HTTP receivers now support setting the `Access-Control-Max-Age` header for CORS caching. (#4492)
- `client.Info` pre-populated for all receivers using common helpers like `confighttp` and `configgrpc` (#4423)

## 🧰 Bug fixes 🧰

- Fix handling of corrupted records by persistent buffer (experimental) (#4475)

Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.41.0