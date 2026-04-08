# thanos v0.12 Release Notes

Source: [v0.12.2](https://github.com/thanos-io/thanos/releases/tag/v0.12.2)

### Fixed

- [#2459](https://github.com/thanos-io/thanos/issues/2459) Compact: Fixed issue with old blocks being marked and deleted in a (slow) loop.
- [#2533](https://github.com/thanos-io/thanos/pull/2515) Rule: do not wrap reload endpoint with `/`. Makes `/-/reload` accessible again when no prefix has been specified.