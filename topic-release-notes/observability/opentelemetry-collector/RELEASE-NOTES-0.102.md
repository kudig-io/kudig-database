# opentelemetry-collector v0.102 Release Notes

Source: [v0.102.1](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.102.1)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.102.1

### This release addresses CVE-2024-36129 ([GHSA-c74f-6mfw-mm4v](https://github.com/open-telemetry/opentelemetry-collector/security/advisories/GHSA-c74f-6mfw-mm4v)) fully.

## End User Changelog

### 🧰 Bug fixes 🧰

- `configrpc`: Use own compressors for zstd (#10323)
   Before this change, the zstd compressor we used didn't respect the max message size. This addresses CVE-2024-36129 (GHSA-c74f-6mfw-mm4v) on `configgrpc`.