# opentelemetry-collector v0.20 Release Notes

Source: [v0.20.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.20.0)

## v0.20.0 Beta

## 🛑 Breaking changes 🛑

- Move `samplingprocessor/probabilisticsamplerprocessor` to `probabilisticsamplerprocessor` (#2392), affects only user who import the code.

## 💡 Enhancements 💡

- `hostmetrics` receiver: Refactor to use metrics metadata utilities (#2405, #2406, #2421)
- Add k8s.node semantic conventions (#2425)

## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.
