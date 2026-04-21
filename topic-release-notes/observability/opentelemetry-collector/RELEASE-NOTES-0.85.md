# opentelemetry-collector v0.85 Release Notes

Source: [v0.85.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.85.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.85.0

### 💡 Enhancements 💡

- `components command`: The "components" command now lists the component's stability levels. (#8289)
  Note that the format of this output is NOT stable and can change between versions.
- `confighttp`: Add option to disable HTTP keep-alives (#8260)

### 🧰 Bug fixes 🧰

- `confmap`: fix bugs of unmarshalling slice values (#4001)
- `exporterhelper`: Stop logging error messages suggesting user to enable `retry_on_failure` or `sending_queue` when they are not available. (#8369)