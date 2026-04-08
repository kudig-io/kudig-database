# opentelemetry-collector v0.141 Release Notes

Source: [v0.141.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.141.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.141.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `pkg/config/confighttp`: Use configoptional.Optional for confighttp.ClientConfig.Cookies field (#14021)

### 💡 Enhancements 💡

- `pkg/config/confighttp`: Setting `compression_algorithms` to an empty list now disables automatic decompression, ignoring Content-Encoding (#14131)
- `pkg/service`: Update semantic conventions from internal telemetry to v1.37.0 (#14232)
- `pkg/xscraper`: Implement xscraper for Profiles. (#13915)

### 🧰 Bug fixes 🧰

- `pkg/config/configoptional`: Ensure that configoptional.None values resulting from unmarshaling are equivalent to configoptional.Optional zero value. (#14218)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pkg/config/configgrpc`: Replace `component.Host` parameter of ToServer/ToClientConn by map of extensions (#13640)
  Components must now pass the map obtained from the host's `GetExtensions` method
  instead of the host itself.
  
  Nil may be used in tests where no middleware or authentication extensions are used.
  
- `pkg/config/confighttp`: Replace `component.Host` parameter of ToServer/ToClient by map of extensions (#13640)
  Components must now pass the map obtained from the host's `GetExtensions` method
  instead of the host itself.
  
  Nil may be used in tests where no middleware or authentication extensions are used.
  

### 🚩 Deprecations 🚩

- `pkg/pdata`: Deprecate profile.Duration() and profile.SetDuration() (#14188)

### 💡 Enhancements 💡

- `pdata/pprofile`: Introduce `MergeTo` method (#14091)
- `pkg/pdata`: Add profile.DurationNano() and profile.SetDurationNano() (#14188)

<!-- previous-version -->
