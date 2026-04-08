# opentelemetry-collector v0.81 Release Notes

Source: [v0.81.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.81.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.81.0

## v0.81.0

### 🛑 Breaking changes 🛑

- `service`: Remove 'service.connectors' featuregate (#7952)

### 💡 Enhancements 💡

- `HTTPServerSettings`: Add zstd support to HTTPServerSettings (#7927)
  This adds ability to decompress zstd-compressed HTTP requests to| all receivers that use HTTPServerSettings.
- `cmd/builder`: Add "--skip-generate" option to make builder skip source generation (#7541)
- `confighttp`: Add support for additional content decoders via `WithDecoder` server option (#7977)
- `connectortest`: Add helpers to aid the construction of `connector.TracesRouter`, `connector.MetricsRouter`, and `connector.LogsRouter` instances to `connectortest`. (#7672)
- `confighttp`: Add `response_headers` configuration option on HTTPServerSettings. It allows for additional headers to be attached to each HTTP response sent to the client (#7328)
- `otlpreceiver, otlphttpexporter, otlpexporter, configgrpc`: Upgrade github.com/mostynb/go-grpc-compression and switch to nonclobbering imports (#7920)
  consumers of this library should not have their grpc codecs overridden
- `otlphttpexporter`: Treat partial success responses as errors (#6686)

### 🧰 Bug fixes 🧰

- `HTTPServerSettings`: Ensure requests with unsupported Content-Encoding return HTTP 400 Bad Request (#7927)