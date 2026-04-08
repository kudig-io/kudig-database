# prometheus v1.2 Release Notes

Source: [v1.2.3](https://github.com/prometheus/prometheus/releases/tag/v1.2.3)

- [BUGFIX] Correctly handle end time before start time in range queries.
- [BUGFIX] Correctly handle empty Regex entry in relabel config.
- [BUGFIX] MOD (`%`) operator doesn't panic with small floating point numbers.
- [BUGFIX] Updated miekg/dns vendoring to pick up upstream bug fixes.
- [ENHANCEMENT] Improved DNS error reporting.
