# thanos v0.39 Release Notes

Source: [v0.39.2](https://github.com/thanos-io/thanos/releases/tag/v0.39.2)

Fixes two issues with the distributed query engine.

Fixed
- [#8374](https://github.com/thanos-io/thanos/pull/8374) Query: fix panic when concurrently accessing annotations map
- [#8375](https://github.com/thanos-io/thanos/pull/8375) Query: fix native histogram buckets in distributed queries

Full Changelog: https://github.com/thanos-io/thanos/compare/v0.39.1...v0.39.2