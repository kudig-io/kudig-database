# thanos v0.3 Release Notes

Source: [v0.3.2](https://github.com/thanos-io/thanos/releases/tag/v0.3.2)

## [v0.3.2](https://github.com/improbable-eng/thanos/releases/tag/v0.3.2) - 2019.03.04

### Added
- [#851](https://github.com/improbable-eng/thanos/pull/851) New read API endpoint for api/v1/rules and api/v1/alerts.
- [#873](https://github.com/improbable-eng/thanos/pull/873) Store: fix set index cache LRU.

### Fixed
- [#833](https://github.com/improbable-eng/thanos/issues/833) Store Gateway matcher regression for intersecting with empty posting.
- [#867](https://github.com/improbable-eng/thanos/pull/867) Fixed race condition in sidecare between reloader and shipper.