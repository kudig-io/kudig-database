# prometheus v1.3 Release Notes

Source: [v1.3.1](https://github.com/prometheus/prometheus/releases/tag/v1.3.1)

This bug-fix release pulls in the fixes from the 1.2.3 release.
- [BUGFIX] Correctly handle empty Regex entry in relabel config.
- [BUGFIX] MOD (`%`) operator doesn't panic with small floating point numbers.
- [BUGFIX] Updated miekg/dns vendoring to pick up upstream bug fixes.
- [ENHANCEMENT] Improved DNS error reporting.
